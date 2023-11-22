#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/syslog.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "protocol.h"
#include "server.h"
#include "medialib.h"
#include "thread_list.h"

extern int udp_socket;
extern struct server_config_st server_config;

static struct programList_st * udp_packet;
static int packet_size = 0;

static pthread_t list_thread_id;

/**
 *@brief    broadcast program list per second
 */
static void * Thread_List_Func(void * param)
{
    static struct timeval time;
    struct sockaddr_in server_addr;

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(atoi(server_config.send_port));
    server_addr.sin_addr.s_addr = inet_addr(server_config.send_address);

    while(1)
    {
        sendto(udp_socket, udp_packet, packet_size, 0, (struct sockaddr *) &server_addr, sizeof(struct sockaddr_in));

        /* sleep a while... */

        time.tv_sec = 1;
        time.tv_usec = 0;

        select(0, NULL, NULL, NULL, &time);
    } 

    pthread_exit(NULL);
}

/**
 *@brief    create program list thread
 *@param    mediaLib:   pointer to mediaLib
 *@retval   0:          OK
 *          -1:         invalid input param
 *          -2:         packet's size too large!
 *          -3:         failed to malloc for packet_list
 */
int Thread_List_Create(void * mediaLib)
{
    if(mediaLib == NULL)
        return -1;

    struct mediaLib_st * ptr = mediaLib;
    
    char * ptr_mediaList = (void *)ptr->list;

    /* statistic and restore packet length  */
    int * length_array = malloc(sizeof(int) * ptr->channel_cnt);

    packet_size += sizeof(channel_id_t);

    for( int i = 0; i < ptr->channel_cnt; i++ )
    {
        length_array[i] = strlen(((struct channelInfo_st *)(ptr_mediaList + i * SIZE_CHANNEL_INFO))->desc);
        
        packet_size += (sizeof(uint16_t) + length_array[i]);
    }

    /* packet will end with "\0\0" */
    packet_size += 2;

    /* check packet length */
    if( packet_size > (MAX_UDP_PACKET_BYTES - sizeof(int)) )
    {
        free(length_array);
        return -2;
    }

    udp_packet = malloc(sizeof(struct programList_st) + packet_size );
    if( udp_packet == NULL )
    {
        free(length_array);
        return -3;
    }

    udp_packet->channel_id = 0;

    char * offset = (void *)udp_packet->info;

    /* merge each channel's description */
    for( int i = 0; i < ptr->channel_cnt; i++ )
    {
        /* description length */
        ((struct channelDesc_st *)offset)->length = htons(length_array[i]);
        
        offset += sizeof(uint16_t);

        /* description */
        memcpy(offset, 
        ((struct channelInfo_st *)(ptr_mediaList + i * SIZE_CHANNEL_INFO))->desc,
        length_array[i]);

        offset += length_array[i];
    }

    /* end with '\0' */
    strcat(offset, "\0\0");

    if( pthread_create(&list_thread_id, NULL, Thread_List_Func, NULL) < 0)
    {
        syslog(LOG_ERR, "Thread_List_Create::pthread_create:%s\n", strerror(errno));
        exit(EXIT_FAILURE); 
    }

    free(length_array);

    return 0;
}

void Thread_List_Destroy(void)
{
    pthread_cancel(list_thread_id);
    pthread_join(list_thread_id, NULL);
}
