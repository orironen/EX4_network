#include "config.h"

unsigned short int calculate_checksum(void *data, unsigned int bytes)
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