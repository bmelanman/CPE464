
#include "udpPDU.h"

int createPDU(uint8_t pduBuff[], uint32_t seqNum, uint8_t flag, uint8_t *payload, int payloadLen) {

    uint16_t pduBuffLen = 0, checksum = 0;
    /* Convert the sequence to network order */
    uint32_t seqNum_NET_ORDER = htonl(seqNum);

    /* Check inputs */
    if (pduBuff == NULL || payload == NULL || payloadLen > 1400) {
        fprintf(stderr, "createPDU() err! Null buffers\n");
        exit(EXIT_FAILURE);
    }

    /* Add the sequence in network order (4 bytes) */
    memcpy(pduBuff, &seqNum_NET_ORDER, 4);
    pduBuffLen += 4;

    /* Set the checksum to 0 for now (2 bytes) */
    memcpy(&pduBuff[pduBuffLen], &checksum, 2);
    pduBuffLen += 2;

    /* Add in the flag (1 byte) */
    memcpy(&pduBuff[pduBuffLen], &flag, 1);
    pduBuffLen++;

    /* Add in the payload */
    memcpy(&pduBuff[pduBuffLen], payload, payloadLen);
    pduBuffLen += payloadLen;

    /* Calculate the payload checksum */
    checksum = in_cksum((unsigned short *) pduBuff, pduBuffLen);

    /* Put the checksum into the PDU */
    memcpy(&pduBuff[4], &checksum, 2);

    return pduBuffLen;
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

void printPDU(uint8_t pduBuff[], int pduLength) {

    uint32_t seqNum;
    uint16_t checksum;
    uint8_t flag, payload[MAX_PAYLOAD_LEN];
    int payloadLen;
    char verifyChecksum[8] = "Valid";

    /* Unpack the PDU */
    unpackPDU(pduBuff, pduLength, &seqNum, &checksum, &flag, payload, &payloadLen);

    /* Verify the checksum */
    if (in_cksum((unsigned short *) pduBuff, pduLength)) {
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
            seqNum, verifyChecksum, flag, payload, payloadLen
    );
}
