
#ifndef PROJECT03_CPE464_WINDOWLIB_H
#define PROJECT03_CPE464_WINDOWLIB_H

#include "networkUtils.h"

#define WINDOW_LOWER (-1)
#define WINDOW_CURRENT (-2)
#define WINDOW_CURRENT_NO_INC (-3)

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

circularQueue_t *createQueue(uint32_t len);

circularWindow_t *createWindow(uint32_t windowSize);

void addQueuePacket(circularQueue_t *queue, packet_t *packet, uint16_t packetLen);

uint16_t getQueuePacket(circularQueue_t *queue, packet_t *packet);

uint32_t peekLastSeq(circularQueue_t *queue);

void addWindowPacket(circularWindow_t *window, packet_t *packet, uint16_t packetLen);

uint16_t getWindowPacket(circularWindow_t *window, packet_t *packet, int pos);

uint8_t checkNextSeq_NO(circularQueue_t *queue, uint32_t seq_NO);

void moveWindow(circularWindow_t *window, uint16_t newLowerIdx);

int checkInputSpaceAvailable(circularWindow_t *window);

int checkWindowSpaceAvailable(circularWindow_t *window);

void freeQueue(circularQueue_t *queue);

void freeWindow(circularWindow_t *window);

#endif /* PROJECT03_CPE464_WINDOWLIB_H */
