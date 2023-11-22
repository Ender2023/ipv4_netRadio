#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syslog.h>
#include <unistd.h>
#include <glob.h>
#include <errno.h>
#include <string.h>

#include "server.h"
#include "medialib.h"
#include "stream_control.h"

#define DEBUG                   0

#define MAX_PATH_BUFFER_SIZE    (256U)
#define MAX_DESC_BUFFER_SIZE    (1024U)

#define TOKEN_CPS               (4 * MP3_BITRATE / 8)
#define TOKEN_BURST             (8 * MP3_BITRATE / 8)

extern struct server_config_st server_config;

static void parseChannelFolder(const char * path, struct channelInfo_st * channel)
{
    glob_t result;
    char path_buffer[MAX_PATH_BUFFER_SIZE];
    char desc_buffer[MAX_DESC_BUFFER_SIZE];

    /* concatenating path and use glob() to find .mp3 files. */
    strncpy(path_buffer, path, MAX_PATH_BUFFER_SIZE);
    
    strcat(path_buffer, "/*.mp3");

    if( glob(path_buffer, 0, NULL, &result) != 0 )
    {
        syslog(LOG_ERR, "parseChannelFolder::glob():%s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    channel->music_file = result.gl_pathv[0];

    /* concatenating path and use glob() to find desc.txt files. */
    strncpy(path_buffer, path, MAX_PATH_BUFFER_SIZE);

    strcat(path_buffer, "/desc.txt");

    if( glob(path_buffer, 0, NULL, &result) != 0 )
    {
        syslog(LOG_ERR, "parseChannelFolder::glob():%s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* open and read description from desc.txt */

    FILE * fp = fopen(path_buffer, "r");

    if(fp == NULL)
    {
        syslog(LOG_ERR, "parseChannelFolder::fopen():%s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    if( fgets(desc_buffer, MAX_DESC_BUFFER_SIZE, fp) == NULL)
    {
        fclose(fp);
        syslog(LOG_ERR, "parseChannelFolder::fgets():%s", strerror(errno));
        exit(EXIT_FAILURE);
    }

    channel->desc = strdup(desc_buffer);

    /* close file */
    fclose(fp);
}




void updateMediaInfo(struct mediaLib_st * mediaLib)
{
    glob_t path;

    if( glob(server_config.mediaLib_path, 0, NULL, &path) != 0 )
    {
        syslog(LOG_ERR, "updateMediaInfo::glob():%s", strerror(errno));
        exit(EXIT_FAILURE);
    }

#if DEBUG == 1
    for( int i = 0; i < path.gl_pathc; i++ )
	{
		puts(path.gl_pathv[i]);
	}
#endif

    /* record channel counts */
    mediaLib->channel_cnt = path.gl_pathc;

    /* malloc for saving each channel's path */
    char * ptr = malloc(sizeof( struct channelInfo_st ) * path.gl_pathc );
    if( ptr == NULL )
    {
        syslog(LOG_ERR, "updateMediaInfo::malloc():%s", strerror(errno));
        exit(EXIT_FAILURE); 
    }

    mediaLib->list = (struct channelInfo_st *) ptr; 

    for( int i = 0; i < path.gl_pathc; i++ )
    {
        ((struct channelInfo_st *)(ptr + i * SIZE_CHANNEL_INFO))->id = i + 1;
        ((struct channelInfo_st *)(ptr + i * SIZE_CHANNEL_INFO))->folder_path = path.gl_pathv[i];

        /* parse and get channel information */
        parseChannelFolder(path.gl_pathv[i], ((struct channelInfo_st *)(ptr + i * SIZE_CHANNEL_INFO)));

        /* assign token bucket for current channel */
        int tbd = tokenBucket_Create(TOKEN_CPS, TOKEN_BURST, true);

        if( tbd < 0 )
        {
            syslog(LOG_ERR, "updateMediaInfo::tokenBucket_Create():bucket list has been full\n");
            exit(EXIT_FAILURE);
        }

        ((struct channelInfo_st *)(ptr + i * SIZE_CHANNEL_INFO))->tbd = tbd;
    }
}
