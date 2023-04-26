
#include "networkUtils.h"

int sendPDU(int clientSocket, uint8_t dataBuffer[], int lengthOfData, uint8_t pduFlag) {

    /* Calculate PDU length */
    uint8_t pduLen = lengthOfData + PDU_HEADER_LEN;
    uint16_t pduLenNetOrd = htons(pduLen);
    uint8_t pduBuff[pduLen];
    ssize_t bytesSent;

    /* Fill the packet */
    memcpy(pduBuff, &pduLenNetOrd, 2);
    memcpy(pduBuff + 2, &pduFlag, 1);
    memcpy(pduBuff + PDU_HEADER_LEN, dataBuffer, lengthOfData);

    /* Send it off */
    bytesSent = send(clientSocket, pduBuff, pduLen, 0);

    /* Error checking */
    if (bytesSent < 0) {
        if (errno == EBADF) {
            bytesSent = 0;
        } else {
            perror("send call");
            exit(EXIT_FAILURE);
        }
    }

    return (int) bytesSent;
}

int recvPDU(int socketNumber, uint8_t dataBuffer[], int bufferSize) {

    uint16_t msgLen;
    ssize_t bytesReceived;

    /* Get the packet length (first 2 bytes) */
    bytesReceived = recv(socketNumber, dataBuffer, PDU_MSG_LEN, MSG_WAITALL);

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
    msgLen = ntohs((dataBuffer[1] << 8) | dataBuffer[0]) - PDU_MSG_LEN;

    /* Check message size */
    if (msgLen > bufferSize) {
        fprintf(stderr, "Given buffer is too small, packMessage size was %d bytes", msgLen);
        exit(EXIT_FAILURE);
    }

    /* Get the rest of the message */
    bytesReceived = (uint16_t) recv(socketNumber, dataBuffer + PDU_MSG_LEN, msgLen, MSG_WAITALL);

    /* Error checking */
    if (bytesReceived < 0) {
        if (errno == ECONNRESET) {
            bytesReceived = 0;
        } else {
            perror("recv call");
            exit(EXIT_FAILURE);
        }
    }

    return (int) bytesReceived;
}
