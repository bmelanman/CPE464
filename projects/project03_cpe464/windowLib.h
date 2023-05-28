
#ifndef PROJECT03_CPE464_WINDOWLIB_H
#define PROJECT03_CPE464_WINDOWLIB_H

#include "networkUtils.h"

#define WINDOW_LOWER (-1)
#define WINDOW_CURRENT (-2)

typedef struct circularQueueStruct {
    packet_t **pktQueue;
    uint16_t *lenQueue;
    uint16_t inputIdx;
    uint16_t outputIdx;
    uint32_t size;
} circularQueue_t;

typedef struct windowStruct {
    uint16_t lower;
    uint16_t current;
    uint16_t upper;
    circularQueue_t *circQueue;
} circularWindow_t;

circularQueue_t *createQueue(uint32_t len, uint16_t bufferLen);

circularWindow_t *createWindow(uint32_t windowSize, uint16_t bufferLen);

void addQueuePacket(circularQueue_t *queue, packet_t *packet, uint16_t packetLen);

uint16_t getQueuePacket(circularQueue_t *queue, packet_t *packet);

void addWindowPacket(circularWindow_t *window, packet_t *packet, uint16_t packetLen);

uint16_t getWindowPacket(circularWindow_t *window, packet_t *packet, int pos);

uint16_t peekQueuePacket(circularQueue_t *queue, packet_t *packet);

uint8_t peekNextSeq_NO(circularQueue_t *queue, uint32_t seq_NO);

void moveWindow(circularWindow_t *window, uint16_t newLowerIdx);

int getWindowSpace(circularWindow_t *window);

int checkWindowOpen(circularWindow_t *window);

void resetCurrent(circularWindow_t *window);

void freeQueue(circularQueue_t *queue);

void freeWindow(circularWindow_t *window);

#endif /* PROJECT03_CPE464_WINDOWLIB_H */
