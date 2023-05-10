
/* Simple UDP Client
 * Created by Bryce Melander in May 2023
 * using modified code from Dr. Hugh Smith
 *  */

#include "rcopy.h"
#include "udpPDU.h"

int main(int argc, char *argv[]) {

    /* IPv6 support added by using an ipv6 struct */
    struct sockaddr_in6 server;
    int socketNum, portNumber;
    float errorRate;

    checkArgs(argc, argv, &portNumber, &errorRate);

    printf("\n");
    socketNum = setupUdpClientToServer(&server, argv[2], portNumber);

    printf("Error rate: %3.2f%%\n\n", errorRate * 100);

    sendErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);

    talkToServer(socketNum, &server);

    close(socketNum);

    return 0;
}

void talkToServer(int socketNum, struct sockaddr_in6 *server) {

    int serverAddrLen = sizeof(struct sockaddr_in6);
    char *ipString = NULL;
    int dataLen, pduLen, cnt = 0;
    uint8_t dataBuff[MAX_PAYLOAD_LEN] = {0}, pduBuff[MAX_PDU_LEN];

    while (dataBuff[0] != '.') {
        dataLen = readFromStdin((char *) dataBuff, MAX_PAYLOAD_LEN);

        pduLen = createPDU(pduBuff, cnt++, 9, dataBuff, dataLen);

        safeSendto(socketNum, pduBuff, pduLen, 0, (struct sockaddr *) server, serverAddrLen);

        safeRecvfrom(socketNum, dataBuff, MAXBUF, 0, (struct sockaddr *) server, &serverAddrLen);

        ipString = ipAddressToString(server);
        printf("Server with ip: %s and port %d said it received %s\n", ipString, ntohs(server->sin6_port), dataBuff);

        printf("\n");
    }
}

int readFromStdin(char *buffer, int buffLen) {
    char aChar = 0;
    int inputLen = 0;

    printf("Enter data: ");

    /* Make sure we don't overflow the input buffer */
    while (inputLen < (buffLen - 1) && aChar != '\n') {

        aChar = (char) getchar();

        if (aChar != '\n') {
            buffer[inputLen] = aChar;
            inputLen++;
        }
    }

    /* Add null termination to the string */
    buffer[inputLen++] = '\0';

    return inputLen;
}

void checkArgs(int argc, char *argv[], int *port, float *errRate) {

    /* check command line arguments  */
    if (argc != 4) {
        fprintf(stderr, "\nusage: ./rcopy error-rate host-name port-number \n");
        exit(EXIT_FAILURE);
    }

    *errRate = strtof(argv[1], NULL);

    /* Check that error rate is in a valid range */
    if (*errRate >= 1.0 || *errRate < 0) {
        fprintf(stderr, "\nErr: error-rate must be 0 ≤ r < 1 \n");
        exit(EXIT_FAILURE);
    }

    *port = (int) strtol(argv[3], NULL, 10);
}





