#ifndef __CLIENT_H__
#define __CLIENT_H__

#include <stdint.h>

#define APP_NAME                "NetRadio"
#define VERSION                 "0.0.1"

#define DEFAULT_RCV_PORT        "2002"
#define DEFAULT_RCV_ADDRESS     "224.0.0.1"
#define DEFAULT_DECODER_CMD     "/usr/bin/mpg123 - > /dev/null 2>&1"
#define DEFAULT_NETCARD         "enp0s3"

struct client_config_st
{
    char * rcv_port;
    char * rcv_address;
    char * decoder_cmd;
    char * netcard;
};


#endif
