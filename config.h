#ifndef _PING_H
#define _PING_H

#define TIMEOUT 2000
#define BUFFER_SIZE 1024
#define SLEEP_TIME 1
#define MAX_REQUESTS 0
#define MAX_RETRY 3
unsigned short int calculate_checksum(void *data, unsigned int bytes);

#endif