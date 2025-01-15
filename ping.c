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
#include <getopt.h>
#include <netinet/icmp6.h>
#include <netinet/ip6.h>

#define MAX_RETRY 3
#define TIMEOUT 10000 // זמן קצוב 10 שניות
#define SLEEP_TIME 1  // זמן שינה בין פינגים
#define BUFFER_SIZE 1024
#define MAX_REQUESTS 4

int main(int argc, char *argv[]) {
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
	struct pollfd fds[1];

	int opt;
	int protocol_type = 0; //
	int count = 0;
	int flood = 0;

	while ((opt = getopt(argc, argv, "a:t:c:f")) != -1)
	{
		switch (opt)
		{
			case 'a':
				if (inet_pton(AF_INET, argv[1], &destination_address4.sin_addr) == 1)
				{
					destination_address4.sin_family = AF_INET; // IPv4
				}
				else if (inet_pton(AF_INET6, argv[1], &destination_address6.sin6_addr) == 1)
				{
					destination_address6.sin6_family = AF_INET6; // IPv6
				}
				else
				{
					fprintf(stderr, "Error: \"%s\" is not a valid IPv4 address\n", argv[1]);
					return -1;
				}
			break;
			case 't':
				protocol_type = atoi(argv[1]);
			if (protocol_type != 4 && protocol_type != 6)
			{
				printf("Invalid number of flag\n");
				return -1;
			}
			printf("Pinging address: %s\n", argv[1]);
			printf("Protocol: IPv%d\n", protocol_type);
			break;
		}
	}
	int sock;
	if (protocol_type == 4)
	{
		sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
		if (sock < 0)
		{
			perror("socket(2)");
			if (errno == EACCES || errno == EPERM)
				fprintf(stderr, "you need to run the program with sudo.\n");
			return 1;
		}
		struct icmphdr icmp_header;
		icmp_header.type = ICMP_ECHO;
		icmp_header.code = 0;
		icmp_header.un.echo.id = htons(getpid());
		seq = 0;

		while (1) {
			memset(buffer, 0, sizeof(buffer));
			icmp_header.un.echo.sequence = htons(seq++);
			icmp_header.checksum = 0;
			memcpy(buffer, &icmp_header, sizeof(icmp_header));
			memcpy(buffer + sizeof(icmp_header), msg, payload_size);
			icmp_header.checksum = calculate_checksum(buffer, sizeof(icmp_header) + payload_size);

			struct icmphdr *pckt_hdr = (struct icmphdr *)buffer;
			pckt_hdr->checksum = icmp_header.checksum;
			struct timeval start, end;
			gettimeofday(&start, NULL);
			if (sendto(sock, buffer, (sizeof(icmp_header) + payload_size), 0, (struct sockaddr *)&destination_address4, sizeof(destination_address4)) <= 0)
			{
				perror("sendto(2)");
				close(sock);
				return 1;
			}
			fds[0].fd = sock;
			fds[0].events = POLLIN;
			fprintf(stdout, "PING %s with %d bytes of data:\n", argv[1], payload_size);
			int ret = poll(fds, 1, TIMEOUT);
			if (ret == 0)
			{
				if (++retries == MAX_RETRY)
				{
					fprintf(stderr, "Request timeout for icmp_seq %d, aborting.\n", seq);
					break;
				}
				fprintf(stderr, "Request timeout for icmp_seq %d, retrying...\n", seq);
				--seq;
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
				struct sockaddr_in source_address;
				memset(buffer, 0, sizeof(buffer));
				memset(&source_address, 0, sizeof(source_address));
				if (recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&source_address, &(socklen_t){sizeof(source_address)}) <= 0)
				{
					perror("recvfrom(2)");
					close(sock);
					return 1;
				}
				retries = 0;
				gettimeofday(&end, NULL);
				struct iphdr *ip_header = (struct iphdr *)buffer;
				struct icmphdr *icmp_header = (struct icmphdr *)(buffer + ip_header->ihl * 4);
				if (icmp_header->type == ICMP_ECHOREPLY)
				{
					float rtt = ((float)(end.tv_usec - start.tv_usec) / 1000) + ((end.tv_sec - start.tv_sec) * 1000);
					fprintf(stdout, "%ld bytes from %s: icmp_seq=%d ttl=%d time=%.2fms\n",
							(ntohs(ip_header->tot_len) - (ip_header->ihl * 4) - sizeof(struct icmphdr)),
							inet_ntoa(source_address.sin_addr),
							ntohs(icmp_header->un.echo.sequence),
							ip_header->ttl,rtt);
					if (seq == MAX_REQUESTS)
						break;
				}
				else
					fprintf(stderr, "Error: packet received with type %d\n", icmp_header->type);
			}
			sleep(SLEEP_TIME);
		}
	}
    else if (protocol_type == 6){
    	sock = socket(AF_INET6, SOCK_RAW, IPPROTO_ICMPV6);
    	if (sock < 0) {
    		perror("socket(2)");
    		if (errno == EACCES || errno == EPERM) {
    			fprintf(stderr, "You need to run the program with sudo.\n");
    		}
    		return 1;
    	}
    	struct icmp6_hdr icmp6_header;
    	icmp6_header.icmp6_type = ICMP6_ECHO_REQUEST;
    	icmp6_header.icmp6_code = 0;
    	icmp6_header.icmp6_id = htons(getpid());
    	seq=0;

		while (1) {
			memcpy(buffer + sizeof(icmp6_header), msg, payload_size);
			memset(buffer, 0, sizeof(buffer));
			icmp6_header.icmp6_seq  = htons(seq++);
			icmp6_header.icmp6_cksum = 0;
			memcpy(buffer, &icmp6_header, sizeof(icmp6_header));
			memcpy(buffer + sizeof(icmp6_header), msg, payload_size);
			icmp6_header.icmp6_cksum = calculate_checksum(buffer, sizeof(icmp6_header) + payload_size);

			struct icmphdr *pckt_hdr = (struct icmphdr *)buffer;
			pckt_hdr->checksum = icmp6_header.icmp6_cksum;
			struct timeval start, end;
			gettimeofday(&start, NULL);
			if (sendto(sock, buffer, sizeof(icmp6_header) + payload_size, 0, (struct sockaddr *)&destination_address6, sizeof(destination_address6)) <= 0) {
				perror("sendto(2)");
				close(sock);
				return 1;
			}

			fds[0].fd = sock;
			fds[0].events = POLLIN;
			fprintf(stdout, "PING %s with %d bytes of data:\n", argv[1], payload_size);
			int ret = poll(fds, 1, TIMEOUT);
			if (ret == 0)
			{
				if (++retries == MAX_RETRY)
				{
					fprintf(stderr, "Request timeout for icmp_seq %d, aborting.\n", seq);
					break;
				}
				fprintf(stderr, "Request timeout for icmp_seq %d, retrying...\n", seq);
				--seq;
				continue;
			}
			else if (ret < 0)
			{
				perror("poll(2)");
				close(sock);
				return 1;
			}
			if (fds[0].revents & POLLIN) {
				struct sockaddr_in6 source_address;
				memset(buffer, 0, sizeof(buffer));
				memset(&source_address, 0, sizeof(source_address));
				if (recvfrom(sock, buffer, sizeof(buffer), 0, (struct sockaddr *)&source_address, &(socklen_t){sizeof(source_address)}) <= 0){
					perror("recvfrom(2)");
					close(sock);
					return 1;
				}
				retries = 0;
				gettimeofday(&end, NULL);
				struct ip6_hdr *ip6_header = (struct ip6_hdr *)buffer;
				struct icmp6_hdr *icmp6_header = (struct icmp6_hdr *)(buffer + sizeof(struct ip6_hdr));
				if (icmp6_header->icmp6_type == ICMP6_ECHO_REPLY)
				{
					float rtt = ((float)(end.tv_usec - start.tv_usec) / 1000) + ((end.tv_sec - start.tv_sec) * 1000);
					char source_ip[INET6_ADDRSTRLEN];
					inet_ntop(AF_INET6, &source_address.sin6_addr, source_ip, sizeof(source_ip));
					fprintf(stdout, "%ld bytes from %s: icmp_seq=%d time=%.2fms\n",payload_size,source_ip,ntohs(icmp6_header.icmp6_seq),rtt);
					if (seq == MAX_REQUESTS)
						break;
				}
				else
					fprintf(stderr, "Error: packet received with type %d\n", icmp6_header->icmp6_type);
			}
			sleep(SLEEP_TIME);
		}
    }

    }



