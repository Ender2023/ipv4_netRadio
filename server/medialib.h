#ifndef __MEDIA_LIB_H__
#define __MEDIA_LIB_H__

#include <pthread.h>

#define MP3_BITRATE         (128 * 1024U )

#define SIZE_CHANNEL_INFO   (sizeof(struct channelInfo_st))

/* total:40 hex:0x28 */
struct channelInfo_st
{
    int         id;             //offset:0  size:4 + 4 padding
    
    char *      folder_path;    //offset:8  size:8
    char *      music_file;     //offset:16 size:8
    char *      desc;           //offset:24 size:8

    int         tbd;            //offset:32 size:4 + 4 padding
};

struct mediaLib_st
{
    int                     channel_cnt;
    struct channelInfo_st * list;
};

void updateMediaInfo(struct mediaLib_st * mediaLib);

#endif

