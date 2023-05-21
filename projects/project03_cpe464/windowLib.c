
#include "windowLib.h"

circularQueue_t *createBuffer(uint16_t size) {

    circularQueue_t *newQueue = malloc(sizeof(circularQueue_t));

    newQueue->packetSize = size;

    return newQueue;
}

windowQueue_t *createWindow(uint32_t windowSize, uint16_t bufferSize) {

    windowQueue_t *newWindow = malloc(windowSize * sizeof(windowQueue_t));

    newWindow->circQueue = createBuffer(bufferSize);

    newWindow->lower = 0;
    newWindow->current = 0;
    newWindow->upper = windowSize;

    newWindow->size = windowSize;

    return newWindow;
}

int addPackets(circularQueue_t *buffer, uint8_t numPackets, FILE *fd) {



    return 0;
}

int moveWindow(windowQueue_t *window, uint16_t newLower, FILE *fd) {



    return 0;
}

void flushBuffer(circularQueue_t *buffer, FILE *fd) {



}

udpPacket_t getPacket(circularQueue_t *buffer, uint16_t idx) {

    return buffer->queue[idx];
}

void incrementCurrent(windowQueue_t *window) {
    window->current++;
}

uint16_t getCurrent(windowQueue_t *window) {
    return window->current;
}

int checkWindowClosed(windowQueue_t *window) {

    if (window->current == window->upper) return 1;

    return 0;
}
