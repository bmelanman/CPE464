
#include "libPDU.h"

int sendPDU(int clientSocket, uint8_t *dataBuffer, int lengthOfData) {

    /* Calculate PDU length */
    int pdu_len = lengthOfData + PDU_HEADER_LEN;
    uint16_t pdu_len_no = htons(pdu_len), bytesSent;
    uint8_t tcp_pdu_buff[pdu_len];

    /* Fill the packet */
    memcpy(tcp_pdu_buff, &pdu_len_no, PDU_HEADER_LEN);
    memcpy(tcp_pdu_buff + PDU_HEADER_LEN, dataBuffer, lengthOfData);

    /* Send it off */
    bytesSent = (uint16_t) send(clientSocket, tcp_pdu_buff, pdu_len, 0);

    /* Error checking */
    if (bytesSent < 0) {
        perror("recv call");
        exit(EXIT_FAILURE);
    }

    return bytesSent;
}

int recvPDU(int socketNumber, uint8_t *dataBuffer, int bufferSize) {

    uint8_t msgLenBuff[2];
    uint16_t msgLen, bytesReceived;

    /* Get the header */
    bytesReceived = (uint16_t) recv(socketNumber, msgLenBuff, PDU_HEADER_LEN, MSG_WAITALL);

    /* Error checking */
    if (bytesReceived < 0) {
        if (errno == ECONNRESET) {
            bytesReceived = 0;
        } else {
            perror("recv call");
            exit(EXIT_FAILURE);
        }
    }

    /* Check if client has disconnected */
    if (bytesReceived == 0) {
        return 0;
    }

    /* Convert network order uint8 array to host order uint16 */
    msgLen = ntohs((msgLenBuff[1] << 8) | msgLenBuff[0]) - PDU_HEADER_LEN;

    /* Check message size */
    if (msgLen > bufferSize) {
        fprintf(stderr, "Given buffer is too small, message size was %d bytes", msgLen);
        exit(EXIT_FAILURE);
    }

    /* Get the rest of the message */
    bytesReceived = (uint16_t) recv(socketNumber, dataBuffer, msgLen, MSG_WAITALL);

    /* Error checking */
    if (bytesReceived < 0) {
        if (errno == ECONNRESET) {
            bytesReceived = 0;
        } else {
            perror("recv call");
            exit(EXIT_FAILURE);
        }
    }

    return bytesReceived;
}



















