
#ifndef PROJECT03_CPE464_WINDOWLIB_H
#define PROJECT03_CPE464_WINDOWLIB_H

#include "networkUtils.h"

typedef struct circularQueueStruct {
    udpPacket_t **pktQueue;
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

void addQueuePacket(circularQueue_t *queue, udpPacket_t *packet, uint16_t packetLen);

uint16_t getQueuePacket(circularQueue_t *queue, udpPacket_t *packet);

void addWindowPacket(circularWindow_t *window, udpPacket_t *packet, uint16_t packetLen);

uint16_t getCurrentPacket(circularWindow_t *window, udpPacket_t *packet);

uint16_t getLowestPacket(circularWindow_t *window, udpPacket_t *packet);

void getSeqPacket(circularWindow_t *window, uint32_t seqNum, udpPacket_t *packet);

udpPacket_t *peekQueuePacket(circularQueue_t *queue);

uint8_t peekNextSeq_NO(circularQueue_t *queue, uint32_t seq_NO);

void moveWindow(circularWindow_t *window, uint16_t newLowerIdx);

void moveCurrentToSeq(circularWindow_t *window, uint16_t seq);

void incrementCurrent(circularWindow_t *window);

int getWindowSpace(circularWindow_t *window);

int checkSendSpace(circularWindow_t *window);

void resetCurrent(circularWindow_t *window);

#endif /* PROJECT03_CPE464_WINDOWLIB_H */
