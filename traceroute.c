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
#include "config.h"

int main(int argc, char *argv[])
{
    int opt;
    char *dest_addr = NULL;
    // find address
    while ((opt = getopt(argc, argv, "a:")) >= 0)
    {
        switch (opt)
        {
        case 'a':
            dest_addr = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s -a <dest-addr>\n", argv[0]);
            return 1;
        }
    }
    if (dest_addr == NULL)
    {
        fprintf(stderr, "Usage: %s -a <dest-addr>\n", argv[0]);
        return 1;
    }
    // set up dest addr
    struct sockaddr_in destination_address;
    memset(&destination_address, 0, sizeof(destination_address));
    destination_address.sin_family = AF_INET;
    if (inet_pton(AF_INET, dest_addr, &destination_address.sin_addr) <= 0)
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
    // create ICMP header
    struct icmphdr icmp_header;
    icmp_header.type = ICMP_ECHO;
    icmp_header.code = 0;
    icmp_header.un.echo.id = htons(getpid());
    // packet seq
    int seq = 0;
    // no of hops
    int hops = 1;
    // time to live
    int ttl = 1;
    // if reached dest
    int reached_dest = 0;
    // create poll structure
    struct pollfd fds[1];
    fds[0].fd = sock;
    fds[0].events = POLLIN;
    printf("traceroute to %s, %d hops max\n", dest_addr, MAX_HOPS);
    while (1)
    {
        // print hop num
        printf("%d ", hops);
        // set buff and icmp header
        memset(buffer, 0, sizeof(buffer));
        icmp_header.un.echo.sequence = htons(seq++);
        icmp_header.checksum = 0;
        // set TTL
        if (setsockopt(sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0)
        {
            perror("getsockopt(2)");
            close(sock);
            return 1;
        }
        // add icmp header
        memcpy(buffer, &icmp_header, sizeof(icmp_header));
        // add payload after
        memcpy(buffer + sizeof(icmp_header), msg, payload_size);
        // calculate checksum
        int checksum = calculate_checksum(buffer, sizeof(icmp_header) + payload_size);
        icmp_header.checksum = checksum;
        struct icmphdr *pckt_hdr = (struct icmphdr *)buffer;
        pckt_hdr->checksum = icmp_header.checksum;
        // initialize source address
        struct sockaddr_in source_address;
        for (size_t i = 0; i < 3; i++)
        {
            struct timeval start, end;
            // get start time
            gettimeofday(&start, NULL);
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
            {
                // print asterisk if timeout
                printf(" * ");
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
                // print source address
                if (i == 0)
                    printf("%s ", inet_ntoa(source_address.sin_addr));
                struct iphdr *ip_header = (struct iphdr *)buffer;
                struct icmphdr *icmp_header = (struct icmphdr *)(buffer + ip_header->ihl * 4);
                // print elapsed time
                if (icmp_header->type == ICMP_ECHOREPLY || icmp_header->type == ICMP_TIME_EXCEEDED)
                    printf("%.3fms ", ((float)(end.tv_usec - start.tv_usec) / 1000) + ((end.tv_sec - start.tv_sec) * 1000));
                if (icmp_header->type == ICMP_ECHOREPLY)
                    reached_dest = 1;
            }
            else
                fprintf(stderr, "Error: packet received with type %d\n", icmp_header.type);
        }
        printf("\n");
        // end condition
        if (hops == MAX_HOPS || reached_dest || ttl >= 64)
            break;
        // increment TTL and hop no
        ttl++;
        hops++;
    }
    close(sock);
    return 0;
}