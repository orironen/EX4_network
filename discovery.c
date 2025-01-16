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
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include "config.h"

// Converts the number of 0 bits in a subnet mask to the binary subnet mask.
uint32_t numToSubnet(int num)
{
    if (num <= 0 || num > 32)
        return 0;
    return ~0U << (32 - num);
}

int main(int argc, char *argv[])
{
    if (argc != 5)
    {
        fprintf(stderr, "Usage: %s -a <dest-addr> -c <subnet-mask>\n", argv[0]);
        return 1;
    }
    char opt;
    char *dest_addr = NULL;
    int subnet_no = -1;
    // find address
    while ((opt = getopt(argc, argv, "a:c:")) >= 0)
    {
        switch (opt)
        {
        case 'a':
            dest_addr = optarg;
            break;
        case 'c':
            subnet_no = atoi(optarg);
            break;
        default:
            fprintf(stderr, "Usage: %s -a <dest-addr> -c <subnet-mask>\n", argv[0]);
            return 1;
        }
    }
    if (dest_addr == NULL || subnet_no == -1)
    {
        fprintf(stderr, "Usage: %s -a <dest-addr> -c <subnet-mask>\n", argv[0]);
        return 1;
    }
    // initialize destination address
    struct sockaddr_in destination_address;
    // set up subnet mask
    uint32_t subnet_mask;
    if ((subnet_mask = numToSubnet(subnet_no)) == 0)
    {
        fprintf(stderr, "Error: \"%d\" is not a valid subnet mask\n", subnet_no);
        return 1;
    }
    // find address range
    int num_of_addr = 1 << (32 - subnet_no);
    in_addr_t addr_range[num_of_addr];
    memset(&addr_range, 0, sizeof(addr_range));
    uint32_t network_addr = ntohl(inet_addr(dest_addr) & ntohl(subnet_mask));
    for (int i = 1; i < num_of_addr; i++)
        addr_range[i] = htonl(network_addr + i);
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
    // create ICMP header
    struct icmphdr icmp_header;
    icmp_header.type = ICMP_ECHO;
    icmp_header.code = 0;
    icmp_header.un.echo.id = htons(getpid());
    // packet seq
    int seq = 0;
    // create poll structure
    struct pollfd fds[1];
    fds[0].fd = sock;
    fds[0].events = POLLIN;
    // print initial message
    printf("scanning %s/%d\n", dest_addr, subnet_no);
    // validate ips in range
    for (int i = 0; i < num_of_addr; i++)
    {
        // set buff and icmp header
        memset(buffer, 0, sizeof(buffer));
        icmp_header.un.echo.sequence = htons(seq++);
        icmp_header.checksum = 0;
        // add icmp header
        memcpy(buffer, &icmp_header, sizeof(icmp_header));
        // add payload after
        memcpy(buffer + sizeof(icmp_header), msg, payload_size);
        // calculate checksum
        int checksum = calculate_checksum(buffer, sizeof(icmp_header) + payload_size);
        icmp_header.checksum = checksum;
        struct icmphdr *pckt_hdr = (struct icmphdr *)buffer;
        pckt_hdr->checksum = icmp_header.checksum;
        // set source address
        memset(&destination_address, 0, sizeof(destination_address));
        destination_address.sin_family = AF_INET;
        destination_address.sin_addr.s_addr = addr_range[i];
        // send packet
        if (sendto(sock, buffer, (sizeof(icmp_header) + payload_size), 0, (struct sockaddr *)&destination_address, sizeof(destination_address)) <= 0)
        {
            perror("sendto(2)");
            close(sock);
            return 1;
        }
        // wait for reply
        int ret = poll(fds, 1, 1000);
        if (ret == 0)
            continue;
        else if (ret < 0)
        {
            perror("poll(2)");
            close(sock);
            return 1;
        }
        // initialize destination address
        struct sockaddr_in source_address;
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
            // print destination address
            struct iphdr *ip_header = (struct iphdr *)buffer;
            struct icmphdr *icmp_header = (struct icmphdr *)(buffer + ip_header->ihl * 4);
            if (icmp_header->type == ICMP_ECHOREPLY)
                printf("%s\n", inet_ntoa(destination_address.sin_addr));
        }
        else
            fprintf(stderr, "Error: packet received with type %d\n", icmp_header.type);
    }
    close(sock);
    printf("Scan Complete!\n");
    return 0;
}