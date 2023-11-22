#ifndef __SERVER_H__
#define __SERVER_H__

#define APP_NAME                "NetRadio_Server"
#define VERSION                 "0.0.1"

#define DEFAULT_SND_ADDRESS     "224.0.0.1"
#define DEFAULT_SND_PORT        "2002"
#define DEFAULT_RUN_STATE       "background"

#define DEFAULT_MEDIA_LIB_PATH  "/var/NetRadio/medialib/channel_*"

struct server_config_st
{
    char * send_address;
    char * send_port;
    char * run_state;
    char * mediaLib_path;
};

#endif
