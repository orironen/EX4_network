#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <getopt.h>
#include "config.h"

int main(int argc, char *argv[])
{
    // find address
    char option[argc];
    getopt(argc, &argv, &option);
    if (argc != 3 || option[0] != "a")
    {
        printf(stderr, "Usage: %s -a <dest-addr>\n", argv[0]);
        return 1;
    }
    // set dest address
    struct sockaddr_in destination_address;
    memset(&destination_address, 0, sizeof(destination_address));
    destination_address.sin_family = AF_INET;
    if (inet_pton(AF_INET, argv[2], &destination_address.sin_addr) <= 0)
    {
        fprintf(stderr, "Error: \"%s\" is not a valid IPv4 address\n", argv[1]);
        return 1;
    }
    // set msg
    char buffer[BUFFER_SIZE] = {0};
    char *msg = "ABCDEFGHIJKLMNOPQRSTUVWXYZ1234567890!@#$^&*()_+{}|:<>?~`-=[]',.";
    int payload_size = strlen(msg) + 1;
    // create socket
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sock < 0)
    {
        perror("socket(2)");
        if (errno == EACCES || errno == EPERM)
            fprintf(stderr, "You need to run the program with sudo.\n");
        return 1;
    }
    int ttl = 1;
    if (setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
    {
        perror("Can't set TTL");
        return 1;
    }
    // create ICMP header
    struct icmphdr icmp_header;
    icmp_header.type = ICMP_ECHO;
    icmp_header.code = 0;
    icmp_header.un.echo.id = htons(getpid());
    int seq = 0;
}