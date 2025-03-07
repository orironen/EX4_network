#ifndef _PING_H
#define _PING_H

#define TIMEOUT 10
#define BUFFER_SIZE 1024
#define SLEEP_TIME 1
#define MAX_REQUESTS 0
#define MAX_RETRY 3
#define MAX_HOPS 30
unsigned short int calculate_checksum(void *data, unsigned int bytes);
#endif