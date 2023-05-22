
#include "windowLib.h"

circularQueue_t *createBuffer() {

    circularQueue_t *newQueue = malloc(sizeof(circularQueue_t));

    newQueue->currentIdx = 0;

    return newQueue;
}

circularWindow_t *createWindow(uint32_t windowSize, uint16_t bufferSize) {

    circularWindow_t *newWindow = malloc(windowSize * sizeof(circularWindow_t));

    newWindow->circQueue = createBuffer();

    newWindow->lower = 0;
    newWindow->current = 0;
    newWindow->upper = windowSize;

    newWindow->size = windowSize;

    return newWindow;
}

void addPacket(circularQueue_t *buffer, udpPacket_t *packet) {

    buffer->queue[buffer->currentIdx] = packet;
//    memcpy(buffer->queue[buffer->currentIdx], packet, sizeof(*packet));

    buffer->currentIdx = (buffer->currentIdx + 1) % CIRC_BUFF_SIZE;

}

int moveWindow(circularWindow_t *window, uint16_t newLower, FILE *fd) {


    return 0;
}

void flushBuffer(circularQueue_t *buffer, FILE *fd) {


}

udpPacket_t *getPacket(circularQueue_t *buffer, uint16_t idx) {

    return buffer->queue[idx];
}

void incrementCurrent(circularWindow_t *window) {
    window->current++;
}

uint16_t getCurrent(circularWindow_t *window) {
    return window->current;
}

int checkWindowSpace(circularWindow_t *window) {

    if (window->circQueue->currentIdx < window->upper) return 1;

    return 0;
}

int checkSendSpace(circularWindow_t *window) {

    if (window->current >= window->upper) return 0;

    return 1;
}
