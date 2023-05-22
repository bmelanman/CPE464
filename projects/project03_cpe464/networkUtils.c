
#include <errno.h>
#include "networkUtils.h"

void *srealloc(void *ptr, size_t size) {
    void *returnValue = NULL;

    if ((returnValue = realloc(ptr, size)) == NULL) {
        printf("Error on realloc (tried for size: %d\n", (int) size);
        exit(EXIT_FAILURE);
    }

    return returnValue;
}

void *scalloc(size_t nmemb, size_t size) {
    void *returnValue = NULL;
    if ((returnValue = calloc(nmemb, size)) == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }
    return returnValue;
}

int safeRecvFrom(int socketNum, void *buf, int len, int flags, struct sockaddr *srcAddr, int addrLen) {

    /* Receive a packet */
    int ret = recvfrom(socketNum, buf, (size_t) len, flags, srcAddr, (socklen_t *) &addrLen);

    /* Check for errors */
    if (ret < 0 && errno != EINTR) {
        perror("recvFrom: ");
        exit(EXIT_FAILURE);
    }

    return ret;
}

int safeSendTo(int socketNum, void *buf, int len, struct sockaddr *srcAddr, int addrLen) {

    /* Send the packet */
    ssize_t ret = sendtoErr(socketNum, buf, len, 0, srcAddr, addrLen);

    /* Check for errors */
    if (ret < 0) {
        perror("sendTo");
        exit(EXIT_FAILURE);
    }

    return (int) ret;
}

void createPDU(udpPacket_t *pduPacket, uint32_t seqNum, uint8_t flag, uint8_t *payload, int payloadLen) {

    uint16_t checksum;

    /* Check inputs */
    if (pduPacket == NULL || payloadLen > 1400) {
        fprintf(stderr, "createPDU() err! Null buffers\n");
        exit(EXIT_FAILURE);
    }

    if (payload == NULL && payloadLen > 0) {
        fprintf(stderr, "createPDU() err! Null buffers\n");
        exit(EXIT_FAILURE);
    }

    /* Add the PDU header length */
    pduPacket->pduLen = PDU_HEADER_LEN;

    /* Add the sequence in network order (4 bytes) */
    pduPacket->seq_NO = htonl(seqNum);

    /* Set the checksum to 0 for now (2 bytes) */
    pduPacket->checksum = 0;

    /* Add in the flag (1 byte) */
    pduPacket->flag = flag;

    /* Add in the payload */
    memcpy(pduPacket->payload, payload, payloadLen);
    pduPacket->pduLen += payloadLen;

    /* Calculate the payload checksum */
    checksum = in_cksum((unsigned short *) pduPacket, pduPacket->pduLen);

    /* Put the checksum into the PDU */
    pduPacket->checksum = checksum;
}
