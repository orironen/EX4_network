#ifndef _PING_H
#define _PING_H

#define TIMEOUT 2000
#define BUFFER_SIZE 1024
#define SLEEP_TIME 1
#define MAX_REQUESTS 0
#define MAX_RETRY 3
#define MAX_HOPS 30
unsigned short int calculate_checksum4(void *data, unsigned int bytes);
unsigned short int calculate_checksum6(const struct ip6_hdr *ip6_header, const void *data, size_t len, uint8_t next_header);
#endif