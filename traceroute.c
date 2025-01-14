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

struct ipheader
{
    unsigned char iph_ihl : 5,       // IP header length
        iph_ver : 4;                 // IP version
    unsigned char iph_tos;           // Type of service
    unsigned short int iph_len;      // IP Packet length (data + header)
    unsigned short int iph_ident;    // Identification
    unsigned short int iph_flag : 3, // Fragmentation flags
        iph_offset : 13;             // Flags offset
    unsigned char iph_ttl : 1;       // Time to Live
    unsigned char iph_protocol : 1;  // Protocol type
    unsigned short int iph_chksum;   // IP datagram checksum
    struct sockaddr iph_sourceip;    // Source IP address
    struct in_addr iph_destip;       // Destination IP address
};

int main(int argc, char *argv[])
{
    // find address
    char option[argc];
    getopt(argc, &argv, &option);
    if (argc != 3 || option[0] != "a")
    {
        fprintf(stderr, "Usage: %s -a <dest-addr>\n", argv[0]);
        return 1;
    }
    // set up dest addr
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
    // create IP header
    struct ipheader iph;
    iph.iph_destip = destination_address.sin_addr;
    if (getsockname(sock, &iph.iph_sourceip, NULL) < 0)
    {
        fprintf(stderr, "Error: Invalid source address.");
        return 1;
    }
    // create ICMP header
    struct icmphdr icmp_header;
    icmp_header.type = ICMP_ECHO;
    icmp_header.code = 0;
    icmp_header.un.echo.id = htons(getpid());
    int seq = 0;
    // create poll structure
    struct pollfd fds[1];
    fds[0].fd = sock;
    fds[0].events = POLLIN;
    printf("traceroute to %s, %d hops max\n", argv[1], MAX_HOPS);
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        icmp_header.un.echo.sequence = htons(seq++);
        icmp_header.checksum = 0;
        iph.iph_chksum = 0;
        // add ip header
        memcpy(buffer, &iph, sizeof(iph));
        // add icmp header after
        memcpy(buffer + sizeof(iph), &icmp_header, sizeof(icmp_header));
        // add payload after
        memcpy(buffer + sizeof(iph) + sizeof(icmp_header), msg, payload_size);
        // calculate checksum
        int checksum = calculate_checksum(buffer, sizeof(iph) + sizeof(icmp_header) + payload_size);
        iph.iph_chksum = checksum;
        icmp_header.checksum = checksum;
    }
}