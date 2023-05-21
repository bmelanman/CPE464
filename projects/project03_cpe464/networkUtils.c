
#include "networkUtils.h"

void *srealloc(void *ptr, size_t size) {
    void *returnValue = NULL;

    if ((returnValue = realloc(ptr, size)) == NULL) {
        printf("Error on realloc (tried for size: %d\n", (int) size);
        exit(-1);
    }

    return returnValue;
}

void *scalloc(size_t nmemb, size_t size) {
    void *returnValue = NULL;
    if ((returnValue = calloc(nmemb, size)) == NULL) {
        perror("calloc");
        exit(-1);
    }
    return returnValue;
}

int safeRecvFrom(int socketNum, void *buf, int len, int flags, struct sockaddr *srcAddr, int *addrLen) {

    /* Receive a packet */
    int ret = recvfrom(socketNum, buf, (size_t) len, flags, srcAddr, (socklen_t *) addrLen);

    /* Check for errors */
    if (ret < 0) {
        perror("recvFrom: ");
        exit(-1);
    }

    return ret;
}

int safeSendTo(int socketNum, void *buf, int len, struct sockaddr *srcAddr, int addrLen) {

    /* Send the packet */
    ssize_t ret = sendtoErr(socketNum, buf, len, 0, srcAddr, addrLen);

    /* Check for errors */
    if (ret < 0) {
        perror("sendTo: ");
        exit(-1);
    }

    return (int) ret;
}

int createPDU(udpPacket_t *pduPacket, uint32_t seqNum, uint8_t flag, uint8_t *payload, int payloadLen) {

    uint16_t checksum, pduLen = PDU_HEADER_LEN;

    /* Check inputs */
    if (pduPacket == NULL || payload == NULL || payloadLen > 1400) {
        fprintf(stderr, "createPDU() err! Null buffers\n");
        exit(EXIT_FAILURE);
    }

    /* Add the sequence in network order (4 bytes) */
    pduPacket->seq_NO = htonl(seqNum);

    /* Set the checksum to 0 for now (2 bytes) */
    pduPacket->checksum = 0;

    /* Add in the flag (1 byte) */
    pduPacket->flag = flag;

    /* Add in the payload */
    memcpy(pduPacket->payload, payload, payloadLen);
    pduLen += payloadLen;

    /* Calculate the payload checksum */
    checksum = in_cksum((unsigned short *) pduPacket, pduLen);

    /* Put the checksum into the PDU */
    pduPacket->checksum = checksum;

    return pduLen;
}

void unpackPDU(uint8_t pduBuff[], int pduLength, uint32_t *seqNum, uint16_t *checksum, uint8_t *flag, uint8_t payload[],
               int *payloadLen) {

    /* Get the sequence and convert back to host order */
    if (seqNum != NULL) {
        memcpy(seqNum, &pduBuff[PDU_SEQ], 4);
        *seqNum = ntohl(*seqNum);
    }

    /* Get the checksum */
    if (checksum != NULL) {
        memcpy(checksum, &pduBuff[PDU_CHK], 2);
    }

    /* Get the flag */
    if (flag != NULL) {
        memcpy(flag, &pduBuff[PDU_FLG], 1);
    }

    /* Get the payload and its length */
    if (payload != NULL && payloadLen != NULL) {
        *payloadLen = pduLength - PDU_HEADER_LEN;
        memcpy(payload, &pduBuff[PDU_PLD], *payloadLen);
    }

}

void printPDU(udpPacket_t *pduPacket, int pduLength) {

    char verifyChecksum[8] = "Valid";

    /* Verify the checksum */
    if (in_cksum((unsigned short *) pduPacket, pduLength)) {
        fprintf(stderr, "Err: PDU checksum is INVALID!\n");
        snprintf(verifyChecksum, 8, "Invalid");
    }

    /* Print PDU data */
    printf(
            "PDU Data Received:         \n"
            "\tSequence Number: %d      \n"
            "\tChecksum: %s             \n"
            "\tFlag: %d                 \n"
            "\tPayload: %s              \n"
            "\tPayload Length: %d       \n",
            ntohl(pduPacket->seq_NO), verifyChecksum, pduPacket->flag,
            pduPacket->payload, pduLength - PDU_HEADER_LEN
    );
}
