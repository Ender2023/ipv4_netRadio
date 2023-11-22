#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <net/if.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>

#include "protocol.h"
#include "client.h"

#define CHAR_PRGMLIST_PTR_CAST(ptr)     ((struct programList_st *) ptr)
#define CHAR_CHANNEL_DESC_PTR_CAST(ptr) ((struct channelDesc_st *) ptr)

extern char *optarg;

/* load up client with default config. */
static struct client_config_st client_config =
{
    .rcv_port = DEFAULT_RCV_PORT,
    .rcv_address = DEFAULT_RCV_ADDRESS,
    .decoder_cmd = DEFAULT_DECODER_CMD,
    .netcard = DEFAULT_NETCARD,
};

/* socket for receive packet */
static int udp_socket;

/**  
 * pipe for data exchange
 * pipe_fd[0]:R 
 * pipe_fd[1]:W
 */
static int pipe_fd[2];

/* record child process's pid */
static pid_t pid;

/**
 *@brief    print help
 */
static void ShowHelp(void)
{
//    puts("usage: netradio [option(s)]");
    fprintf(stdout, "usage: %s [option(s)]", APP_NAME);

    puts("support options [defaults in brackets]:");
    puts(" -P, --port[=2002]        specify a new port of server");
    puts(" -i, --ip[=224.0.0.1]     specify a new ip address of server");
    puts(" -p, --player[=mpg123]    specify a new player");
    puts(" -n, --netcard            specify the netcard");
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
static void parse_cmdline(int argc, char ** argv)
{
    struct option long_option[] =
    {
        {"port",    required_argument,  NULL,   'P'},
        {"ip",      required_argument,  NULL,   'i'},
        {"player",  required_argument,  NULL,   'p'},
        {"netcard", required_argument,  NULL,   'n'},
        {"help",    no_argument,        NULL,   'h'},
        {"version", no_argument,        NULL,   'v'},
        {0,         0,                  0,       0}
    };

    int opt_index;
    int opt_retval;

    while(1)
    {
        opt_retval = getopt_long(argc, argv, "P:i:p:n:hv", long_option, &opt_index);

        /* all command-line options have been parsed */
        if(opt_retval == -1)
            break;

        switch(opt_retval)
        {
            case 'P':
            {
                client_config.rcv_port = optarg;
                break;
            }

            case 'i':
            {
                client_config.rcv_address = optarg;
                break;
            }

            case 'n':
            {
                client_config.netcard = optarg;
                break;
            }

            case 'v':
            {
                ShowVersion();
                exit(EXIT_SUCCESS);
            }

            case 'p':
            {
                client_config.decoder_cmd = optarg;
                break;
            }

            case 'h':
            {
                ShowHelp();
                exit(EXIT_SUCCESS);
            }

            case '?':
            {
                exit(EXIT_FAILURE);
            }
        }
    }
}

/**
 *@brief    release udp_socket promptly when main process exit 
 */
static void udp_socket_exit(void)
{
    close(udp_socket);
}

/**
 *@brief    setup socket for client
 */
static void setup_socket(void)
{
    udp_socket = socket(AF_INET, SOCK_DGRAM, 0);

    if( udp_socket < 0 )
    {
        perror("setup_socket::socket()");
        exit(EXIT_FAILURE);
    }

    atexit(udp_socket_exit);

    /* join a multicast group  */
    struct ip_mreqn multigroup;

    inet_pton(AF_INET, "0.0.0.0", &multigroup.imr_address);                     //specify local address 
    inet_pton(AF_INET, client_config.rcv_address, &multigroup.imr_multiaddr);   //specify multicast group address
    multigroup.imr_ifindex = if_nametoindex(client_config.netcard);             //specify the net card to join multifroup

    if(setsockopt(udp_socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &multigroup, sizeof(struct ip_mreqn)) < 0)
    {
        perror("setsockopt::IP_ADD_MEMBERSHIP()");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in local_address;

    /* bind local address with udp_socket */
    local_address.sin_family = AF_INET;
    inet_pton(AF_INET, "0.0.0.0", &local_address.sin_addr);
    local_address.sin_port = htons(atoi(client_config.rcv_port));

    if( bind(udp_socket, (void *)&local_address, sizeof(struct sockaddr_in)) < 0)
    {
        perror("setup_socket::bind()");
        exit(EXIT_FAILURE);
    }
}

/**
 *@brief    release pipe promptly when main process exit 
 */
static void pipe_exit(void)
{
    close(pipe_fd[0]);
    close(pipe_fd[1]);
}

/**
 *@brief    create pipe, then data can be passed from parent to child.
 *@note     anonymous pipe,which means child must inherit from parent!
 */
static void create_pipe(void)
{
    if(pipe(pipe_fd) < 0)
    {
        perror("pipe()");
        exit(EXIT_FAILURE);
    }

    atexit(pipe_exit);
}

/**
 *@brief    porcess decoder
 */
static void Process_decoder(void)
{
    /* child do not need to write pipe. */
    close(pipe_fd[1]);

    /* child do not need to read socket */
    close(udp_socket);

    /* set pipe_fd[0] as child's stdin */
    dup2(pipe_fd[0], 0);

    /* close original pipe_fd[0],then only stdin is pipe_fd[0] */
    if(pipe_fd[0] > 0)
        close(pipe_fd[0]);
    
    /* use shell to parse command "DEFAULT_DECODER_CMD" */
    execl("/bin/sh", "sh", "-c", DEFAULT_DECODER_CMD, NULL);

    /* fail to execute application */
    perror("execl");
}

/**
 *@brief    print welcome title
 */
static void welcomeTitle(void)
{
    printf("welcome to use net radio V%s\n", VERSION);
    printf("searching program list...\n");
}

/**
 *@brief    print program list information
 *@param    list: pointer to the list which has been received
 */
static void printProgramList(struct programList_st * list)
{
    char        buffer[1024];
    char *      pos = (void*) list;
    uint16_t    channel_cnt = 0;

    /* get length of first channel's description. */
    uint16_t desc_len = ntohs(CHAR_PRGMLIST_PTR_CAST(pos)->info->length);

    /* point to first channel's description. */
    pos += sizeof(uint16_t) + sizeof(uint16_t);

    printf("===========CHANNEL    LIST===============\n");

    while(1)
    {
        /* seperate current channel's information from data packet */
        memcpy(buffer, pos, desc_len);
        buffer[desc_len] = '\0';
        
        /* print current channel's information */
        printf("[%d]:%s", ++channel_cnt, buffer);

        /* point to next channel's "length" info */
        pos += desc_len;

        /* get length of next channel's description. */
        desc_len = ntohs(CHAR_CHANNEL_DESC_PTR_CAST(pos)->length);

        /* point to next channel's description */
        pos += sizeof(uint16_t);

        /* judge "end" symbol */
        if(desc_len == 0)
            break;
    }

     printf("=========================================\n");
}

/**
 *@brief    select channel
 *@retval   the channel id user selected.
 */
static int SelectChannel(void)
{
    int channel;
    char buffer[16];

    while(1)
    {
        printf("select channel:");

        scanf("%[0-9]", buffer);
        channel = atoi(buffer);

        if( channel < 1 || channel > 255 )
        {
            printf("invalid input value\n");
            continue;
        }

        return channel;
    }
}

/**
 *@brief    make sure " size" counts of data in "buf" must been written into "dst "
 *@param    dst:    dst
 *@param    buf:    source buffer
 *@param    size:   size
 *@retval   the counts has been written to dst
 */
static int writen(int dst, const void * buf, ssize_t size)
{
    int pos = 0;
    int fail_cnt = 0;

    while(size > 0)
    {
        int ret = write(dst, buf + pos, size);

        if(ret < 0)
        {
            if( errno != EINTR )
            {
                perror("write");
                exit(EXIT_FAILURE);   
            }
            continue;
        }

        if(ret == 0)
        {
            fail_cnt ++;
            if(fail_cnt == 5)
                break;
        }

        size -= ret;
        pos += ret;
    }

    return pos;
}

/**
 *@brief    Porcess main,receive mp3 data from network,then pass them to sub process(mp3 decoder).
 */
static void Process_main(void)
{
    struct sockaddr_in server_address;
    socklen_t address_len = sizeof(struct sockaddr_in);


    char recv_buffer[MAX_UDP_PACKET_BYTES];
    struct programList_st * programList = (struct programList_st *) recv_buffer;


    ssize_t packet_len;

    welcomeTitle();

    /* receive program list */
    while(1)
    {
        packet_len = recvfrom(udp_socket, &recv_buffer, MAX_UDP_PACKET_BYTES, 0, (void *)&server_address, &address_len);

        if(packet_len == -1)
        {
            perror("recvfrom()");
            exit(EXIT_FAILURE);
        }

        /* bad packet,continue */
        if( packet_len < sizeof(struct programList_st) )
            continue;

        /* not program list channel,continue */
        if( programList->channel_id != PROGRAM_LIST_CHANNEL )
            continue;
        
        break;
    }


    printProgramList(programList);
    

    int select_id = SelectChannel();

    struct sockaddr_in sender_address;

    struct channelData_st * channel_data = (struct channelData_st *)recv_buffer;
    
    /* receive program data */
    while(1)
    {
        packet_len = recvfrom(udp_socket, recv_buffer, MAX_UDP_PACKET_BYTES, 0, (void *)&sender_address, &address_len);

        /* packet source address does not match the server's ipv4 address */
        if( server_address.sin_addr.s_addr != sender_address.sin_addr.s_addr )
            continue;

        /* packet source port does not match the server port */
        if( server_address.sin_port != sender_address.sin_port )
            continue;

        /* bad packet */
        if( packet_len < sizeof(struct channelData_st) )
            continue;

        /* is the channel user want? */
        if( ntohs(channel_data->channel_id) != select_id )
            continue;

        /* get the audio stream's length */
        packet_len -= sizeof(channel_id_t);

        /* write to player */
        int ret = writen(pipe_fd[1], channel_data->data, packet_len);
        if( packet_len != ret )
        {
            perror("writen()");
            exit(EXIT_FAILURE);
        }
    }
}

/**
 *@brief    reboot decoder if MP3 decorder become a ZOMBIE !!!!
 */
static void childReboot(int s)
{
    wait(NULL);

    pid = fork();
    
    if( pid < 0 )
    {
        perror("fork()");
        exit(EXIT_FAILURE);
    }

    /* child process */
    if( pid == 0 )
    {
        Process_decoder();

        exit(EXIT_FAILURE);
    }
}

/**
 *@brief    Create a client
 *@param    agrc:       counts of arg
 *@param    argv:       value of args
 *@retval   0:          OK
 *@retval   *:          error
 */
int main(int argc, char ** argv)
{
    /* parse option from command line. */
    parse_cmdline(argc, argv);

    /* set up socket. */
    setup_socket();

    /* create pipe */
    create_pipe();

    /* fork a child */
    pid = fork();
    
    if( pid < 0 )
    {
        perror("fork()");
        exit(EXIT_FAILURE);
    }

    /* child process */
    if( pid == 0 )
    {
        Process_decoder();

        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, childReboot);

    /* parent process */
    Process_main();

    exit(EXIT_SUCCESS);
}
