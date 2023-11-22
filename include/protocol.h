#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#include <stdint.h>
#include <sys/types.h>

#define MULTICAST_ADDRESS       "224.0.0.1"
#define MULTICAST_PORT          "2002"

#define IP_HEADER_BYTES         (20U)
#define UDP_HEADER_BYTES        (8U)

#define MAX_UDP_PACKET_BYTES    (65536U - IP_HEADER_BYTES - UDP_HEADER_BYTES)

#define PROGRAM_LIST_CHANNEL    (0U)

typedef uint16_t channel_id_t;

struct channelDesc_st
{
    int16_t                 length;              //record  length of this chennel's description
    uint8_t                 info[0];
}__attribute__((packed));

struct programList_st
{
    channel_id_t            channel_id;         //for program list,channel id must be 0.
    struct channelDesc_st   info[0];            //list info,contain all channel's information.
}__attribute__((packed));

struct channelData_st
{
    channel_id_t            channel_id;         //for music channel,channel id must not be 0!!!
    uint8_t                 data[0];
}__attribute__((packed));

#endif
