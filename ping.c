#include <stdio.h>           // Standard input/output definitions
#include <arpa/inet.h>       // Definitions for internet operations (inet_pton, inet_ntoa)
#include <netinet/in.h>      // Internet address family (AF_INET, AF_INET6)
#include <netinet/ip.h>      // Definitions for internet protocol operations (IP header)
#include <netinet/ip_icmp.h> // Definitions for internet control message protocol operations (ICMP header)
#include <poll.h>            // Poll API for monitoring file descriptors (poll)
#include <errno.h>           // Error number definitions. Used for error handling (EACCES, EPERM)
#include <string.h>          // String manipulation functions (strlen, memset, memcpy)
#include <sys/socket.h>      // Definitions for socket operations (socket, sendto, recvfrom)
#include <sys/time.h>        // Time types (struct timeval and gettimeofday)
#include <unistd.h>          // UNIX standard function definitions (getpid, close, sleep)
#include "config.h"          // Header file for the program (calculate_checksum function and some constants)
#include <stdlib.h>

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <destination_ip>\n", argv[0]);
        return 1;

    }
    struct sockaddr_in destination_address4;
    struct sockaddr_in6 destination_address6;
    char buffer[BUFFER_SIZE] = {0};
    char *msg = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$^&*()_+{}|:<>?~`-=[]',.";
    int payload_size = strlen(msg) + 1;
    int retries = 0;
    memset(&destination_address4, 0, sizeof(destination_address4));
    memset(&destination_address6, 0, sizeof(destination_address6));
    int seq = 0;

    int opt;
    int kind = 0;

    while ((opt = getopt(argc, argv, "a:t:c:f")) != -1)
    {
        switch (opt)
        {
        case 'a':
            if (inet_pton(AF_INET, argv[1], &destination_address4.sin_addr) == 1)
            {
                destination_address4.sin_family = AF_INET; // IPv4
                kind = 4;
            }
            else if (inet_pton(AF_INET6, argv[1], &destination_address6.sin6_addr) == 1)
            {
                destination_address6.sin6_family = AF_INET6; // IPv6
                kind = 6;
            }
            else
            {
                printf("Invalid IP address: %s\n", argv[1]);
                return -1;
            }
            break;
        case 't':
            kind = atoi(argv[1]);
            if (kind != 4 && kind != 6)
            {
                printf("Invalid number of flag\n");
                return -1;
            }
        }
    }
    int sock;
    if (kind == 4)
    {
        sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
        destination_address4.sin_family = AF_INET;
    }
    else
    {
        sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
        destination_address6.sin6_family = AF_INET6;
    }
    if (sock < 0)
    {
        perror("socket(2)");
        return -1;
    }
}