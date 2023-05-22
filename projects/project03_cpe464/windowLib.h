
#ifndef PROJECT03_CPE464_WINDOWLIB_H
#define PROJECT03_CPE464_WINDOWLIB_H

#include "networkUtils.h"

#define CIRC_BUFF_SIZE 32

typedef struct circularQueueStruct {
    udpPacket_t *queue[CIRC_BUFF_SIZE];
    uint16_t currentIdx;
}circularQueue_t;

typedef struct windowStruct {
    uint32_t size;
    uint16_t lower;
    uint16_t current;
    uint16_t upper;
    circularQueue_t *circQueue;
} circularWindow_t;

circularQueue_t *createBuffer();

circularWindow_t *createWindow(uint32_t windowSize, uint16_t bufferSize);

void addPacket(circularQueue_t *buffer, udpPacket_t *packet);

int checkWindowSpace(circularWindow_t *window);

//int checkSendSpace(circularWindow_t *window);

#endif /* PROJECT03_CPE464_WINDOWLIB_H */
