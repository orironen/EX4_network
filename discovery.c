#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>

// Converts the number of 0 bits in a subnet mask to the binary subnet mask.
uint32_t numToSubnet(int num)
{
    if (num < 0 || num > 32)
        return 0;
    return num == 0 ? 0 : (~0U << (32 - num));
}

int main(int argc, char *argv[])
{
    char opt;
    char *dest_addr = NULL;
    int subnet_no = NULL;
    // find address
    while ((opt = getopt(argc, argv, "a:")) >= 0)
    {
        switch (opt)
        {
        case 'a':
            dest_addr = optarg;
            break;
        default:
            fprintf(stderr, "Usage: %s -a <dest-addr> -c <subnet-mask>\n", argv[0]);
            return 1;
        }
    }
    while ((opt = getopt(argc, argv, "c:")) >= 0)
    {
        switch (opt)
        {
        case 'c':
            subnet_no = atoi(optarg);
        default:
            fprintf(stderr, "Usage: %s -a <dest-addr> -c <subnet-mask>\n", argv[0]);
            return 1;
        }
    }
    if (dest_addr == NULL || subnet_no == NULL)
    {
        fprintf(stderr, "Usage: %s -a <dest-addr> -c <subnet-mask>\n", argv[0]);
        return 1;
    }
    // set up dest addr
    struct sockaddr_in destination_address;
    memset(&destination_address, 0, sizeof(destination_address));
    destination_address.sin_family = AF_INET;
    if (inet_pton(AF_INET, dest_addr, &destination_address.sin_addr) <= 0)
    {
        fprintf(stderr, "Error: \"%s\" is not a valid IPv4 address\n", dest_addr);
        return 1;
    }
    // set up subnet mask
    uint32_t subnet_mask;
    if (subnet_mask = numToSubnet(subnet_no) == 0)
    {
        fprintf(stderr, "Error: \"%d\" is not a valid subnet mask\n", subnet_no);
    }
}