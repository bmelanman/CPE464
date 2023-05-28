
#include "windowLib.h"

circularQueue_t *createQueue(uint32_t len, uint16_t bufferLen) {

    /* Allocate space for the new struct */
    circularQueue_t *newQueue = scalloc(1, sizeof(circularQueue_t));

    /* Allocate space for the queue */
    newQueue->pktQueue = scalloc(1, sizeof(packet_t *) * len);
    newQueue->lenQueue = scalloc(1, sizeof(uint16_t) * len);
    newQueue->size = len;

    /* Allocate each queue entry now instead of later */
    for (int i = 0; i < len; ++i) {
        newQueue->pktQueue[i] = scalloc(1, bufferLen);
    }

    /* Set variables to zero */
    newQueue->inputIdx = 0;
    newQueue->outputIdx = 0;

    return newQueue;
}

void addQueuePacket(circularQueue_t *queue, packet_t *packet, uint16_t packetLen) {

    uint16_t idx = queue->inputIdx % queue->size;

    /* The buffer is circular, check is there is available space */
    if (queue->inputIdx == (queue->outputIdx + queue->size)) return;
    else if (queue->inputIdx > (queue->outputIdx + queue->size)) { // TODO: Error checking, remove
        fprintf(stderr, "Somehow, the input index has moved past the size of the buffer and data is likely corrupt.\n");
        exit(EXIT_FAILURE);
    }

    /* Copy in the packet */
    memcpy(queue->pktQueue[idx], packet, packetLen);

    /* Store the PDU's length */
    queue->lenQueue[idx] = packetLen;

    /* Increment the index */
    queue->inputIdx++;
}

uint16_t peekQueuePacket(circularQueue_t *queue, packet_t *packet) {

    /* Get the packet length from the fifo */
    uint16_t len = queue->lenQueue[queue->outputIdx % queue->size];

    /* Check if the buffer is empty */
    if ((queue->outputIdx + 1) > queue->inputIdx) return 0;

    /* Copy the packet over */
    memcpy(packet, queue->pktQueue[queue->outputIdx % queue->size], len);

    return len;
}

uint16_t getQueuePacket(circularQueue_t *queue, packet_t *packet) {

    /* Get the packet */
    uint16_t len = peekQueuePacket(queue, packet);

    /* Increment the location of the fifo output */
    queue->outputIdx++;

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
            start = (++queue->outputIdx) % queue->size;

        } else {
            /* The packet was not found */
            return 0;
        }
    }

    /* The packet was not found, and all packets in the queue have been flushed */
    return 0;
}

void freeQueue(circularQueue_t *queue) {

    /* Free each packet */
    for (uint32_t i = 0; i < queue->size; ++i) {
        free(queue->pktQueue[i]);
    }

    /* Free the queue struct */
    free(queue);
}

circularWindow_t *createWindow(uint32_t windowSize, uint16_t bufferLen) {

    circularWindow_t *newWindow = scalloc(1, sizeof(circularWindow_t));

    newWindow->circQueue = createQueue(windowSize, bufferLen);

    newWindow->lower = 0;
    newWindow->current = 0;
    newWindow->upper = windowSize;

    return newWindow;
}

void addWindowPacket(circularWindow_t *window, packet_t *packet, uint16_t packetLen) {
    addQueuePacket(window->circQueue, packet, packetLen);
}

uint16_t getWindowPacket(circularWindow_t *window, packet_t *packet, int pos) {

    uint16_t idx, len;

    /* Parse input for special flags */
    if (pos == WINDOW_LOWER) {
        idx = window->lower % window->circQueue->size;

    } else if (pos == WINDOW_CURRENT) {
        idx = window->current % window->circQueue->size;
        window->circQueue->outputIdx++;
        window->current++;

    } else if (pos < window->lower || window->upper < pos) {
        /* Error checking */
        fprintf(stderr, "\nPacket %d has been requested, however lower is %d and upper is %d\n",
                pos, window->lower, window->upper);
        exit(EXIT_FAILURE);

    } else {
        /* Otherwise, use the pos as an index */
        idx = pos % window->circQueue->size;

    }

    /* Get the packet length */
    len = window->circQueue->lenQueue[idx];

    /* Copy the packet contents */
    memcpy(packet, window->circQueue->pktQueue[idx], len);

    return len;
}

void moveWindow(circularWindow_t *window, uint16_t newLowerIdx) {

    window->lower = newLowerIdx;
    window->upper = newLowerIdx + window->circQueue->size;
}

void resetCurrent(circularWindow_t *window) {
    window->current = window->lower;
}

int getWindowSpace(circularWindow_t *window) {

    if (window->circQueue->inputIdx < window->upper) return 1;

    return 0;
}

int checkWindowOpen(circularWindow_t *window) {

    if (window->current < window->upper) return 1;

    return 0;
}

void freeWindow(circularWindow_t *window) {

    freeQueue(window->circQueue);

    free(window);

}

