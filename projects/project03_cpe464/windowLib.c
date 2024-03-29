
#include "windowLib.h"

circularQueue_t *createQueue(uint32_t len) {

    uint32_t i;

    /* Allocate space for the new struct */
    circularQueue_t *newQueue = (circularQueue_t *) scalloc(1, sizeof(circularQueue_t));

    /* Allocate space for the queue */
    newQueue->pktQueue = (packet_t **) scalloc(len, sizeof(packet_t *));
    newQueue->lenQueue = (uint16_t *) scalloc(len, sizeof(uint16_t));
    newQueue->size = len;

    /* Allocate each queue entry now instead of later */
    for (i = 0; i < len; ++i) {
        newQueue->pktQueue[i] = initPacket();
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

    /* Copy in the packet */
    memcpy(queue->pktQueue[idx], packet, MAX_PDU_LEN);

    /* Store the PDU's length */
    queue->lenQueue[idx] = packetLen;

    /* Increment the index */
    queue->inputIdx++;
}

uint16_t getQueuePacket(circularQueue_t *queue, packet_t *packet) {

    uint16_t idx = queue->outputIdx % queue->size;
    uint16_t len = queue->lenQueue[idx];

    /* Check if the fifo is empty */
    if (queue->outputIdx >= queue->inputIdx) return 0;

    /* Copy the packet over */
    memcpy(packet, queue->pktQueue[idx], MAX_PDU_LEN);

    /* Increment the location of the fifo output */
    queue->outputIdx++;

    return len;
}

uint8_t checkNextSeq_NO(circularQueue_t *queue, uint32_t seq_NO) {

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

uint32_t peekLastSeq(circularQueue_t *queue) {

    /* Get the most recently added packet's location */
    uint8_t idx = (queue->inputIdx - 1) % queue->size;

    return ntohl(queue->pktQueue[idx]->seq_NO);
}

void freeQueue(circularQueue_t *queue) {

    uint32_t i;

    /* Free each packet */
    for (i = 0; i < queue->size; ++i) {
        free(queue->pktQueue[i]);
    }

    /* Free the arrays */
    free(queue->pktQueue);
    free(queue->lenQueue);

    /* Free the queue struct */
    free(queue);
}

circularWindow_t *createWindow(uint32_t windowSize) {

    circularWindow_t *newWindow = (circularWindow_t *) scalloc(1, sizeof(circularWindow_t));

    newWindow->circQueue = createQueue(windowSize);

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

    } else if (pos == WINDOW_CURRENT_NO_INC) {
        idx = window->current % window->circQueue->size;

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
    memcpy(packet, window->circQueue->pktQueue[idx], MAX_PDU_LEN);

    return len;
}

void moveWindow(circularWindow_t *window, uint16_t newLowerIdx) {

    window->lower = newLowerIdx;
    window->upper = newLowerIdx + window->circQueue->size;
}

int checkInputSpaceAvailable(circularWindow_t *window) {

    if (window->circQueue->inputIdx < window->upper) return 1;

    return 0;
}

int checkWindowSpaceAvailable(circularWindow_t *window) {

    if (window->current < window->upper) return 1;

    return 0;
}

void freeWindow(circularWindow_t *window) {

    freeQueue(window->circQueue);

    free(window);

}

