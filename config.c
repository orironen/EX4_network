#include <netinet/in.h>
#include <netinet/ip6.h>
#include "config.h"

uint16_t calculate_checksum6(const struct ip6_hdr *ip6_header, const void *data, size_t len, uint8_t next_header)
{
    uint32_t sum = 0;
    const uint16_t *addr = (const uint16_t *)&ip6_header->ip6_src;

    // Sum source and destination addresses
    for (int i = 0; i < 16; i++)
        sum += *addr++;

    // Add the payload length and next header fields
    sum += htons(len);
    sum += htons(next_header);

    // Sum the payload
    const uint16_t *payload = (const uint16_t *)data;
    while (len > 1)
    {
        sum += *payload++;
        len -= 2;
    }

    // Add left-over byte, if any
    if (len > 0)
        sum += *(const uint8_t *)payload;

    // Fold 32-bit sum to 16 bits
    while (sum >> 16)
        sum = (sum & 0xffff) + (sum >> 16);

    return ~sum;
}

unsigned short int calculate_checksum4(void *data, unsigned int bytes)
{
    unsigned short int *data_pointer = (unsigned short int *)data;
    unsigned int total_sum = 0;

    // Main summing loop.
    while (bytes > 1)
    {
        total_sum += *data_pointer++; // Some magic pointer arithmetic.
        bytes -= 2;
    }

    // Add left-over byte, if any.
    if (bytes > 0)
        total_sum += *((unsigned char *)data_pointer);

    // Fold 32-bit sum to 16 bits.
    while (total_sum >> 16)
        total_sum = (total_sum & 0xFFFF) + (total_sum >> 16);

    // Return the one's complement of the result.
    return (~((unsigned short int)total_sum));
}