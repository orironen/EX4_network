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

int main(int argc, char *argv[])
{
    char option;
    getopt(argc, argv, option);
    if (argc != 3 || option != "a")
    {
        printf(stderr, "Usage: %s -a <dest-addr>\n", argv[0]);
        return 1;
    }
    struct sockaddr_in destination_address;
    memset(&destination_address, 0, sizeof(destination_address));
}