
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

int safeRecvFrom(int socketNum, void *buf, int len, addrInfo_t *srcAddr) {

    /* Receive a packet */
    ssize_t ret = recvfromErr(
            socketNum, buf, (size_t) len, 0,
            srcAddr->addrInfo,
            (socklen_t *) &srcAddr->addrLen
    );

    /* Check for errors */
    if (ret < 0 && errno != EINTR) {
        perror("recvFrom: ");
        exit(EXIT_FAILURE);
    }

    return (int) ret;
}

int safeSendTo(int socketNum, void *buf, int len, addrInfo_t *dstInfo) {

    /* Send the packet */
    ssize_t ret = sendtoErr(
            socketNum, buf, len, 0,
            dstInfo->addrInfo,
            dstInfo->addrLen
    );

    /* Check for errors */
    if (ret < 0) {
        perror("sendTo");
        exit(EXIT_FAILURE);
    }

    return (int) ret;
}

addrInfo_t *initAddrInfo() {

    addrInfo_t *a = scalloc(1, sizeof(addrInfo_t));

    a->addrLen = sizeof(struct sockaddr_in6);

    /* Ensure enough space is allocated for later typecasting */
    a->addrInfo = scalloc(1, a->addrLen);

    a->addrInfo->sa_family = AF_INET6;

    return a;
}

udpPacket_t *initPacket(uint16_t bufferLen) {

    /* Allocate space for the header plus the payload, which is a Flexible Array Member */
    return (udpPacket_t *) scalloc(1, sizeof(udpPacket_t) + (sizeof(uint8_t) * bufferLen));
}

int createPDU(udpPacket_t *pduPacket, uint16_t bufferLen, uint32_t seqNum,
              uint8_t flag, uint8_t *payload, int payloadLen) {

    uint16_t checksum, pduLen = PDU_HEADER_LEN + payloadLen;

    /* Check inputs */
    if (pduPacket == NULL) {
        fprintf(stderr, "createPDU() err! Null packet struct\n");
        exit(EXIT_FAILURE);
    } else if (payloadLen > 1400) {
        fprintf(stderr, "createPDU() err! Payload len > 1400\n");
        exit(EXIT_FAILURE);
    } else if (payload == NULL && payloadLen > 0) {
        fprintf(stderr, "createPDU() err! Null payload with non-zero length\n");
        exit(EXIT_FAILURE);
    }

    /* Add the sequence in network order (4 bytes) */
    pduPacket->seq_NO = htonl(seqNum);

    /* Set the checksum to 0 for now (2 bytes) */
    pduPacket->checksum = 0;

    /* Add in the flag (1 byte) */
    pduPacket->flag = flag;

    /* Clear old data */
    memset(pduPacket->payload, 0, bufferLen);

    /* Add in the payload */
    memcpy(pduPacket->payload, payload, payloadLen);

    /* Calculate the payload checksum */
    checksum = in_cksum((unsigned short *) pduPacket, pduLen);

    /* Put the checksum into the PDU */
    pduPacket->checksum = checksum;

    return pduLen;
}
