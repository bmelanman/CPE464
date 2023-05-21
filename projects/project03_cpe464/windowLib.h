
#ifndef PROJECT03_CPE464_WINDOWLIB_H
#define PROJECT03_CPE464_WINDOWLIB_H

#include "networkUtils.h"

#define BUFF_SIZE 100

typedef struct circularQueueStruct {
    udpPacket_t queue[BUFF_SIZE];
    uint16_t packetSize;
}circularQueue_t;

typedef struct windowStruct {
    uint32_t size;
    uint16_t lower;
    uint16_t current;
    uint16_t upper;
    circularQueue_t *circQueue;
} windowQueue_t;

#endif /* PROJECT03_CPE464_WINDOWLIB_H */
