
#include "windowLib.h"

circularQueue_t *createQueue(uint32_t len) {

    /* Allocate space for the new struct */
    circularQueue_t *newQueue = malloc(sizeof(circularQueue_t));

    /* Allocate space for the queue */
    newQueue->pktQueue = malloc(sizeof(udpPacket_t *) * len);
    newQueue->lenQueue = malloc(sizeof(uint16_t) * len);
    newQueue->size = len;

    /* Set variables to zero */
    newQueue->inputIdx = 0;
    newQueue->outputIdx = 0;

    return newQueue;
}

void addQueuePacket(circularQueue_t *queue, udpPacket_t *packet, uint16_t packetLen) {

    uint16_t idx = queue->inputIdx % queue->size;

    /* The buffer is circular, check is there is available space */
    if (queue->inputIdx == (queue->outputIdx + queue->size)) return;
    else if (queue->inputIdx > (queue->outputIdx + queue->size)) {
        // TODO: Error checking, remove
        fprintf(stderr, "Somehow, the input index has moved past the size of the buffer and data is likely corrupt.\n");
        exit(EXIT_FAILURE);
    }

    /* Make space for the packet */
    queue->pktQueue[idx] = malloc(packetLen);

    /* Copy in the packet */
    memcpy(queue->pktQueue[idx], packet, packetLen);

    /* Store the PDU's length */
    queue->lenQueue[idx] = packetLen;

    /* Increment the index */
    queue->inputIdx++;
}

udpPacket_t *peekQueuePacket(circularQueue_t *queue) {
    /* Returns the packet at the output index without moving on to the next */
    return queue->pktQueue[(queue->outputIdx - 1) % queue->size];
}

uint16_t getQueuePacket(circularQueue_t *queue, udpPacket_t *packet) {

    uint16_t len = queue->lenQueue[(queue->outputIdx - 1) % queue->size];

    /* Check if buffer is empty */
    if ((queue->outputIdx + 1) > queue->inputIdx) return 0;

    /* Return the packet at the queue output, then move on to the next */
    queue->outputIdx++;

    memcpy(packet, queue->pktQueue[(queue->outputIdx - 1) % queue->size], len);

    return len;
}

uint8_t peekNextSeq_NO(circularQueue_t *queue, uint32_t seq_NO) {

    uint32_t start = queue->outputIdx % queue->size;

    /* Make sure there is a packet to read */
    if (queue->pktQueue[start] == NULL) return 0;

    /* Go through the queue to remove any packets lower than the requested sequence */
    while (queue->outputIdx < queue->inputIdx) {

        if (queue->pktQueue[start]->seq_NO == seq_NO) {
            /* Return true if the packet is found */
            return 1;

        } else if (queue->pktQueue[start]->seq_NO < seq_NO) {
            /* Flush the packet if it is lower */
            queue->outputIdx++;
            start = queue->outputIdx % queue->size;

        } else {
            /* The packet was not found */
            return 0;
        }
    }

    /* The packet was not found, and all packets in the queue have been flushed */
    return 0;
}

circularWindow_t *createWindow(uint32_t windowSize) {

    circularWindow_t *newWindow = malloc(sizeof(circularWindow_t));

    newWindow->circQueue = createQueue(windowSize);

    newWindow->lower = 0;
    newWindow->current = 0;
    newWindow->upper = windowSize;

    return newWindow;
}

void addWindowPacket(circularWindow_t *window, udpPacket_t *packet, uint16_t packetLen) {
    addQueuePacket(window->circQueue, packet, packetLen);
}

uint16_t getCurrentPacket(circularWindow_t *window, udpPacket_t *packet) {

    uint16_t len = window->circQueue->lenQueue[window->current % window->circQueue->size];

    memcpy(packet, window->circQueue->pktQueue[window->current % window->circQueue->size], len);

    window->circQueue->outputIdx++;

    return len;
}

udpPacket_t *getSeqPacket(circularWindow_t *window, uint32_t seqNum) {

    udpPacket_t *p;

    if (seqNum < window->lower || window->upper < seqNum) {
        // TODO: Error checking, remove
        fprintf(stderr,
                "\nPacket %d has been requested, however lower is %d and upper is %d\n",
                seqNum, window->lower, window->upper);
        exit(EXIT_FAILURE);
    }

    p = malloc(MAX_PDU_LEN);

    memset(p, 0, MAX_PDU_LEN);
    memcpy(p, window->circQueue->pktQueue[seqNum % window->circQueue->size], MAX_PDU_LEN);

    return p;
}

void moveWindow(circularWindow_t *window, uint16_t newLowerIdx) {

    window->lower = newLowerIdx;
    window->upper = newLowerIdx + window->circQueue->size;
}

void moveCurrentToSeq(circularWindow_t *window, uint16_t seq) {

    /* Make sure the given seq is within the current window boundaries */
    if (seq < window->lower) window->current = window->lower;
    else if (window->upper < seq) window->current = window->upper;
    else window->current = seq;
}

void incrementCurrent(circularWindow_t *window) {

    /* Increment the current index */
    window->current++;

    /* Don't go above the upper index */
    if (window->current > window->upper) window->current = window->upper;
}

int getWindowSpace(circularWindow_t *window) {

    if (window->circQueue->inputIdx < window->upper) return 1;

    return 0;
}

int checkSendSpace(circularWindow_t *window) {

    if (window->current < window->upper) return 1;

    return 0;
}
