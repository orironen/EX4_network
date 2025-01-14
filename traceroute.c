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
    unsigned char iph_ttl;           // Time to Live
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
    iph.iph_ttl = 1;
    // create ICMP header
    struct icmphdr icmp_header;
    icmp_header.type = ICMP_ECHO;
    icmp_header.code = 0;
    icmp_header.un.echo.id = htons(getpid());
    int seq = 0;
    int hops = 0;
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
        struct icmphdr *pckt_hdr = (struct icmphdr *)buffer;
        pckt_hdr->checksum = icmp_header.checksum;
        // set time intervals between each hop
        float packettimes[3];
        memset(&packettimes, 0, sizeof(packettimes));
        // initialize source address
        struct sockaddr_in source_address;
        for (size_t i = 0; i < 3; i++)
        {
            struct timeval start, end;
            // get start time
            gettimeofday(&start, NULL);
            // send packet
            if (sendto(sock, buffer, (sizeof(iph) + sizeof(icmp_header) + payload_size), 0, (struct sockaddr *)&destination_address, sizeof(destination_address)) <= 0)
            {
                perror("sendto(2)");
                close(sock);
                return 1;
            }
            // wait for reply
            int ret = poll(fds, 1, TIMEOUT);
            if (ret == 0)
            {
                fprintf(stderr, "Request timeout for icmp_seq %d, ignoring.\n", seq);
                continue;
            }
            else if (ret < 0)
            {
                perror("poll(2)");
                close(sock);
                return 1;
            }
            if (fds[0].revents & POLLIN)
            {
                // get source
                memset(&source_address, 0, sizeof(source_address));
                memset(buffer, 0, sizeof(buffer));
                if (recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&source_address, &(socklen_t){sizeof(source_address)}) <= 0)
                {
                    perror("recvfrom(2)");
                    close(sock);
                    return 1;
                }
                // get end time
                gettimeofday(&end, NULL);
                struct iphdr *ip_header = (struct iphdr *)buffer;
                struct icmphdr *icmp_header = (struct icmphdr *)(buffer + ip_header->ihl * 4);
                if (icmp_header->type == ICMP_ECHOREPLY)
                    packettimes[i] = ((float)(end.tv_usec - start.tv_usec) / 1000) + ((end.tv_sec - start.tv_sec) * 1000);
            }
            else
                fprintf(stderr, "Error: packet received with type %d\n", icmp_header.type);
        }
        printf("%s ", source_address);
        for (size_t i = 0; i < 3; i++)
        {
            if (packettimes[i] > 0)
                printf("%.3fms ", packettimes[i]);
            else
                printf("* ");
        }
        printf("\n");
        hops++;
        iph.iph_ttl++;
        if (hops == MAX_HOPS || source_address.sin_addr.s_addr == destination_address.sin_addr.s_addr)
            break;
        sleep(SLEEP_TIME);
    }
    close(sock);
    return 0;
}