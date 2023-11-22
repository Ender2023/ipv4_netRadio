#include <stdio.h>
#include <stdlib.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <syslog.h>
#include <net/if.h>

#include "server.h"
#include "medialib.h"
#include "protocol.h"
#include "thread_list.h"
#include "thread_channel.h"

extern char *optarg;

struct server_config_st server_config =
{
    .send_address = DEFAULT_SND_ADDRESS,
    .send_port = DEFAULT_SND_PORT,
    .run_state = DEFAULT_RUN_STATE,
    .mediaLib_path = DEFAULT_MEDIA_LIB_PATH,
};

int udp_socket;

/**
 *@brief    print help
 */
static void showHelp(void)
{
    fprintf(stdout, "usage: %s [option(s)]", APP_NAME);

    puts("support options [defaults in brackets]:");
    puts(" -P, --port[=2002]        specify a new port of server");
    puts(" -i, --ip[=224.0.0.1]     specify a new ip address of server");
    puts(" -f, --foreground         make server run in foreground,default in background");
    puts(" -h, --help               show this help document and exit");
    puts(" -v, --version            print name + version");
}

/**
 *@brief    print software version
 */
static void ShowVersion(void)
{
    fprintf(stdout, "%s %s\n", APP_NAME, VERSION);
}

/**
 *@brief    parse the option from command line
 */
static void parseCommandLine(int argc, char ** argv)
{
    int opt_index = 0;
    struct option long_option[] =
    {
        {"ip",          required_argument, NULL, 'i'},
        {"port",        required_argument, NULL, 'p'},
        {"foreground",  no_argument ,      NULL, 'f'},
        {"help",        no_argument ,      0,    'h'},
        {"version",     no_argument ,      0,    'v'},
        {0,0,0,0},
    };

    int res = getopt_long(argc, argv, "i:p:fhv", long_option, &opt_index);
    while(1)
    {
        /* all command-line options have been parsed */
        if( res == -1 )
            break;

        switch( res )
        {
            case 'i':
            {
                server_config.send_address = optarg;
                break;
            }

            case 'p':
            {
                server_config.send_port = optarg;
                break;
            }

            case 'f':
            {
                server_config.run_state = "foreground";
                break;
            }

            case 'h':
            {
                showHelp();
                exit(EXIT_SUCCESS);
            }

            case 'v':
            {
                ShowVersion();
                exit(EXIT_SUCCESS);
            }

            case '?':
            {
                exit(EXIT_FAILURE);
            }

        }

        break;
    }
}

/**
 *@brief    make server run in background
 */
static void daemonize(void)
{
    pid_t pid = fork();

    if( pid < 0 )
    {
        perror("fork");
        exit(EXIT_FAILURE);
    }

    /* child,daemonize */
    if(pid == 0)
    {
        int fd = open("/dev/null", O_RDWR);

        if(fd < 0)
        {
            perror("open");
            exit(EXIT_FAILURE);
        }

        dup2(fd, 0);    //close stdin,and duplicate current fd to stdin
        dup2(fd, 1);    //close stdout,and duplicate current fd to stdout
        dup2(fd, 2);    //close stderr,and duplicate current fd to stderr

        if( fd > 2 )    //fd > 2,means fd is a extra file descritors
            close(fd);

        umask(0);

        setsid();
        
        chdir("/");
    }

    /* parent,exit */
    exit(EXIT_SUCCESS);
}

/**
 *@brief    setup socket for client
 */
static void setupSocket(void)
{
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

    if( udp_socket < 0 )
    {
        syslog(LOG_ERR, "socket():%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    /* allow reuse address if address had been bound */
    int optval = 1;    

    if( setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0 )
    {
        syslog(LOG_ERR, "setsockopt.SO_REUSEADDR():%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

    struct ip_mreqn multigroup;

    inet_pton(AF_INET, "0.0.0.0", &multigroup.imr_address);
    inet_pton(AF_INET, server_config.send_address, &multigroup.imr_multiaddr);
    multigroup.imr_ifindex = if_nametoindex("enp0s3");

    if(setsockopt(udp_socket, IPPROTO_IP, IP_MULTICAST_IF, &multigroup, sizeof(struct ip_mreqn)) < 0)
    {
        syslog(LOG_ERR, "setsockopt::IP_MULTICAST_IF():%s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
}

/**
 *@brief    Create a client
 *@param    agrc:       counts of arg
 *@param    argv:       value of args
 *@retval   0:      OK
 *@retval   *:      error
 */
int main(int argc, char ** argv)
{
    parseCommandLine(argc, argv);

    if( strcmp(server_config.run_state, DEFAULT_RUN_STATE) == 0 )
        daemonize();

    setupSocket();

    struct mediaLib_st media_lib;

    updateMediaInfo(&media_lib);

    int ret = Thread_List_Create(&media_lib);

    switch( -ret )
    {
        case 1: syslog(LOG_ERR, "Thread_List_Create():invalid input param\n");break;
        case 2: syslog(LOG_ERR, "Thread_List_Create():packet's size too large!\n");break;
        case 3: syslog(LOG_ERR, "Thread_List_Create():failed to malloc for packet_list\n");break;
    }

    Thread_Channel_Create(&media_lib);

    while(1);


    exit(EXIT_SUCCESS);
}
