
#include "libPDU.h"

int sendPDU(int clientSocket, uint8_t *dataBuffer, int lengthOfData) {

    int pdu_len = lengthOfData + PDU_HEADER_LEN;
    uint16_t pdu_len_no = htons(pdu_len);
    uint8_t tcp_pdu_buff[pdu_len];

    memcpy(tcp_pdu_buff, &pdu_len_no, PDU_HEADER_LEN);
    memcpy(tcp_pdu_buff + PDU_HEADER_LEN, dataBuffer, lengthOfData);

    return safeSend(clientSocket, tcp_pdu_buff, pdu_len, 0);
}

int recvPDU(int socketNumber, uint8_t *dataBuffer, int bufferSize) {

    uint8_t msgLenBuff[2];
    uint16_t msgLen;

    if (safeRecv(socketNumber, msgLenBuff, PDU_HEADER_LEN, MSG_WAITALL) == 0) {
        return 0;
    }

    /* Convert network order uint8 array to host order uint16 */
    msgLen = ntohs((msgLenBuff[1] << 8) | msgLenBuff[0]) - PDU_HEADER_LEN;

    /* Check message size */
    if ((msgLen) > bufferSize) {
        fprintf(stderr, "Given buffer is too small, message size was %d bytes", msgLen);
        exit(EXIT_FAILURE);
    }

    return safeRecv(socketNumber, dataBuffer, msgLen, MSG_WAITALL);
}



















