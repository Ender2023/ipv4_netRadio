#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <stdint.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>

#include "server.h"
#include "stream_control.h"

#define MAX_BUCKET_COUNT    1024

#define STREAM_DELAY_S      1
#define STREAM_DELAY_US     500000

enum bucket_state 
{
    NO_USE = 0,
    RUNNING,
    PAUSE,
};

struct tokenBucket_st
{
    int               burst;     //upper limit of token
    int               token;     //current token number
    int               CPS;       //decode rate 
    enum bucket_state state;     //record current bucket's run state

    pthread_mutex_t   mutex;
    pthread_cond_t    cond;

};

static pthread_t thread_id;

static struct tokenBucket_st bucketList[MAX_BUCKET_COUNT];

static pthread_once_t first_init;
static pthread_mutex_t mutex_list = PTHREAD_MUTEX_INITIALIZER;

static void * timer_thread(void * param)
{
    static struct timeval time;

    while(1)
    {
        pthread_mutex_lock(&mutex_list);

        for(int i = 0; i < MAX_BUCKET_COUNT; i++)
        {
            if( bucketList[i].state == RUNNING )
            {
                pthread_mutex_lock(&bucketList[i].mutex);

                /* token overload ? */
                if( bucketList[i].token + bucketList[i].CPS > bucketList[i].burst )
                    bucketList[i].token = bucketList[i].burst;
                else
                    bucketList[i].token += bucketList[i].CPS;

                pthread_cond_broadcast(&bucketList[i].cond);
                pthread_mutex_unlock(&bucketList[i].mutex);
            }
        }

        pthread_mutex_unlock(&mutex_list);

        /* sleep a while... */

        time.tv_sec = STREAM_DELAY_S;
        time.tv_usec = STREAM_DELAY_US;

        select(0, NULL, NULL, NULL, &time);
    }

    pthread_exit(NULL);
}

static void tokenBucket_Init(void)
{
    for(int i = 0; i < MAX_BUCKET_COUNT; i++)
    {
        bucketList[i].state = NO_USE;
    }

    if ( pthread_create(&thread_id, NULL, timer_thread, NULL) < 0 )
    {
        fprintf(stderr, "pthread_create():%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

/**
 *@brief    create a token bucket
 *@param    tbd:    token bucket handle
 *@retval   0~MAX_BUCKET_COUNT:         free pos in list.
 *          -1:                         bucket list has been full
 */
static int findFreePos_unlocked(void)
{
    for(int i = 0; i < MAX_BUCKET_COUNT; i++)
    {
        if( bucketList[i].state != NO_USE )
            continue;

        return i;
    }

    return -1;
}

/**
 *@brief    create a token bucket
 *@param    CPS:                    Decording rate
 *@param    burst:                  upper limit of bucket
 *@param    runnow:                 running immediatly?
 *@retval   0~MAX_BUCKET_COUNT:     OK
 *          -1:                     bucket list has been full 
 */
int tokenBucket_Create(int CPS, int burst, bool runnow)
{
    pthread_once(&first_init, tokenBucket_Init);

    int pos;

    pthread_mutex_lock(&mutex_list);

    pos = findFreePos_unlocked();
    
    if( pos < 0 )
    {
        pthread_mutex_unlock(&mutex_list);
        return -1;
    }

    bucketList[pos].state = PAUSE;

    pthread_mutex_unlock(&mutex_list);

    bucketList[pos].burst = burst;
    bucketList[pos].CPS = CPS;
    bucketList[pos].token = 0U;

    pthread_mutex_init(&bucketList[pos].mutex, NULL);
    pthread_cond_init(&bucketList[pos].cond, NULL);

    if(runnow)
        bucketList[pos].state = RUNNING;

    return pos;
}

/**
 *@brief    destroy a token bucket
 *@param    tbd:    token bucket handle
 *@retval   0:      OK
 *          -1:     invalid param
 *          -2:     try to use unregistered bucket
 */
int tokenBucket_Destruct(int tbd)
{
    /* invalid param */
    if( tbd < 0 )
        return -1;

    pthread_mutex_lock(&mutex_list);

    /* try to use unregistered bucket */
    if( bucketList[tbd].state == NO_USE )
    {
        pthread_mutex_unlock(&mutex_list);
        return -2;
    }

    bucketList[tbd].state = NO_USE;

    pthread_mutex_unlock(&mutex_list);

    pthread_mutex_destroy(&bucketList[tbd].mutex);
    pthread_cond_destroy(&bucketList[tbd].cond);

    return 0;
}

/**
 *@brief    fetch size count token from bucket,bucket's handle is tbf
 *@param    tbf:    token bucket handle
 *@param    size:   fetch size count token
 *@retval   0~size: OK
 *          -1:     try to use unregistered bucket
 */
int tokenBucket_fetchToken(int tbd, int size)
{
    if( bucketList[tbd].state == NO_USE )
        return -1;

    pthread_mutex_lock(&bucketList[tbd].mutex);

    while(1)
    {
        if( bucketList[tbd].token == 0 )
        {
            pthread_cond_wait(&bucketList[tbd].cond, &bucketList[tbd].mutex);
        }

        break;
    }

    /* fetch all token out */
    if( size > bucketList[tbd].token )
    {
        int tmp = bucketList[tbd].token;

        bucketList[tbd].token = 0;

        pthread_mutex_unlock(&bucketList[tbd].mutex);

        return tmp;
    }

    /* fetch "size" counts token out */
    bucketList[tbd].token -= size;

    pthread_mutex_unlock(&bucketList[tbd].mutex);

    return size;
}

/**
 *@brief    fetch size count token from bucket,bucket's handle is tbf
 *@param    tbd:    token bucket handle
 *@param    size:   fetch size count token
 *@retval   0~size: OK
 *          -1:     size <= 0
 *          -2:     try to use unregistered bucket
 */
int tokenBucket_returnToken(int tbd, int size)
{
    if( size <= 0 )
        return -1;
    
    if( bucketList[tbd].state == NO_USE )
        return -2;

    pthread_mutex_lock(&bucketList[tbd].mutex);
    
    if( bucketList[tbd].token + size >= bucketList[tbd].burst )
        bucketList[tbd].token = bucketList[tbd].burst;
    
    bucketList[tbd].token += size;

    pthread_cond_broadcast(&bucketList[tbd].cond);
    pthread_mutex_unlock(&bucketList[tbd].mutex);

    return size;
}

/**
 *@brief    pause bucket's token update
 *@param    tbd:    token bucket handle
 *@retval   0:      OK
 *          -1:     size <= 0
 *          -2:     try to use unregistered bucket
 */
int tokenBucket_Pause(int tbd)
{
    if( tbd < 0 )
        return -1;

    if( bucketList[tbd].state == NO_USE )
        return -2;

    bucketList[tbd].state = PAUSE;

    return 0;
}

/**
 *@brief    resume bucket's token update
 *@param    tbd:    token bucket handle
 *@retval   0:      OK
 *          -1:     size <= 0
 *          -2:     try to use unregistered bucket
 */
int tokenBucket_Resume(int tbd)
{
    if( tbd < 0 )
        return -1;

    if( bucketList[tbd].state == NO_USE )
        return -2;

    bucketList[tbd].state = RUNNING;

    return 0;
}
