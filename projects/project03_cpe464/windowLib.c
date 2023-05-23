
#include "windowLib.h"

circularQueue_t *createBuffer() {

    circularQueue_t *newQueue = malloc(sizeof(circularQueue_t));

    newQueue->inputIdx = 0;

    return newQueue;
}

circularWindow_t *createWindow(uint32_t windowSize, uint16_t bufferSize) {

    circularWindow_t *newWindow = malloc(sizeof(circularWindow_t));

    newWindow->circQueue = createBuffer();

    newWindow->lower = 0;
    newWindow->current = 0;
    newWindow->upper = windowSize;

    newWindow->size = windowSize;

    return newWindow;
}

void addPacket(circularQueue_t *buffer, udpPacket_t *packet) {

    uint16_t packetLen = packet->pduLen + 1;

    buffer->queue[buffer->inputIdx % CIRC_BUFF_SIZE] = malloc(packetLen);

    memcpy(buffer->queue[buffer->inputIdx % CIRC_BUFF_SIZE], packet, packetLen);

    buffer->inputIdx++;

}

udpPacket_t *getQueuePacket(circularQueue_t *buffer) {

    buffer->outputIdx++;

    return buffer->queue[(buffer->outputIdx - 1) % CIRC_BUFF_SIZE];
}

int readQueuePacketSeq(circularQueue_t *buffer) {

    if (buffer->queue[buffer->inputIdx % CIRC_BUFF_SIZE] == NULL) return -1;

    return ntohl(buffer->queue[buffer->inputIdx % CIRC_BUFF_SIZE]->seq_NO);
}

int bufferEmpty(circularQueue_t *buffer) {

    if (buffer->inputIdx == 0 && buffer->queue[0] == NULL) return 1;

    return 0;
}

udpPacket_t *getCurrentPacket(circularWindow_t *window) {
    return window->circQueue->queue[window->current % CIRC_BUFF_SIZE];
}

void moveWindow(circularWindow_t *window, uint16_t n) {

    /* Remove old entries */
    for (int i = 0; i < n; ++i) {
        free(window->circQueue->queue[(i + window->lower) % CIRC_BUFF_SIZE]);

        window->circQueue->queue[(i + window->lower) % CIRC_BUFF_SIZE] = NULL;
    }

    window->lower += n;
    window->upper += n;
}

void decrementCurrent(circularWindow_t *window, uint16_t n) {

    window->current -= n;

    if (window->current < window->lower) window->current = window->lower;

}

void incrementCurrent(circularWindow_t *window) {
    window->current++;
}

int getIncrement(circularWindow_t *window) {
    return (int) window->lower;
}

int getWindowSpace(circularWindow_t *window) {

    if (window->circQueue->inputIdx < window->upper) return 1;

    return 0;
}

int checkSendSpace(circularWindow_t *window) {

    if (window->current < window->upper) return 1;

    return 0;
}
