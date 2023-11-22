#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syslog.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>


#include "server.h"
#include "medialib.h"
#include "thread_channel.h"
#include "stream_control.h"
#include "protocol.h"

#define FETCH_TOKEN_SIZE    (2 * MP3_BITRATE / 8)

extern int udp_socket;
extern struct server_config_st server_config;

struct thread_param_st
{
    int                  channel_id;
    struct mediaLib_st * mediaLib;
};

static pthread_t channel_thread_id[128];

static void * Thread_Channel_Func(void * thread_param)
{
    /* get param */
    char * mediaList = (char *) ((struct thread_param_st *)thread_param)->mediaLib->list;
    int channel_id = ((struct thread_param_st *)thread_param)->channel_id;

    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(server_config.send_port));
    server_addr.sin_addr.s_addr = inet_addr(server_config.send_address);

    struct channelData_st * packet = malloc(MAX_UDP_PACKET_BYTES);
    if( packet == NULL )
    {
        syslog(LOG_ERR, "Thread_Channel_Func::malloc():%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    packet->channel_id = htons(channel_id + 1);

    int fd = open(((struct channelInfo_st *)mediaList + channel_id)->music_file, O_RDONLY);
    if(fd < 0)
    {
        syslog(LOG_ERR, "Thread_Channel_Func::open():%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    while(1)
    {
        int size = tokenBucket_fetchToken(((struct channelInfo_st *)mediaList + channel_id)->tbd, FETCH_TOKEN_SIZE );
        if(size < 0)
        {
            syslog(LOG_ERR, "Thread_Channel_Func::tokenBucket_fetchToken():try to use unregistered bucket\n");
            exit(EXIT_FAILURE);
        }

        int len = read(fd, packet->data, size);

        if(len < 0)
        {
            /* if the blockage is interrupted by a signal(fake error) */
            if(errno == EINTR)
            {
                continue;
            }

            /* else it's a real fault */
            syslog(LOG_ERR, "Thread_Channel_Func::read():%s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }

        if(len == 0)
        {
            break;
        }

        /* if token rest */
        if( size - len > 0 )
        {
            tokenBucket_returnToken(((struct channelInfo_st *)mediaList + channel_id)->tbd, (size - len) );
        }

        sendto(udp_socket, packet, size + sizeof(channel_id_t), 0, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in));
    }

    free(packet);

    pthread_exit(NULL);
}

/**
 *@brief    create program thread
 *@param    mediaLib:   pointer to mediaLib
 */
void Thread_Channel_Create(void * mediaLib)
{
    struct mediaLib_st * ptr = mediaLib;

    for( int i = 0; i < ptr->channel_cnt; i++ )
    {
        struct thread_param_st * param = malloc(sizeof(struct thread_param_st));

        param->mediaLib = mediaLib;
        param->channel_id = i;

        if( pthread_create(&channel_thread_id[i], NULL, Thread_Channel_Func, param ) < 0)
        {
            syslog(LOG_ERR, "Thread_Channel_Create::pthread_create:%s\n", strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
}

void Thread_Channel_Destroy(void * mediaLib)
{
    struct mediaLib_st * ptr = mediaLib;

    for(int i = 0; i < ptr->channel_cnt; i++ )
    {
        pthread_cancel(channel_thread_id[i]);
        pthread_join(channel_thread_id[i], NULL);
    }
}
