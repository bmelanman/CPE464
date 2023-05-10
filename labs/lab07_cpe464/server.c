
/* Simple UDP Server
 * Created by Bryce Melander in May 2023
 * using modified code from Dr. Hugh Smith
 *  */

#include "server.h"
#include "udpPDU.h"

int main(int argc, char *argv[]) {
    int socketNum, portNumber;
    float errorRate;

    checkArgs(argc, argv, &portNumber, &errorRate);

    printf("\n");
    socketNum = udpServerSetup(portNumber);

    printf("Error rate: %3.2f%%\n\n", errorRate * 100);

    sendErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);

    processClient(socketNum);

    close(socketNum);

    return 0;
}

void processClient(int socketNum) {
    int pduLen;
    uint8_t dataBuff[MAX_PDU_LEN];
    struct sockaddr_in6 client;
    int clientAddrLen = sizeof(client);

    dataBuff[0] = '\0';
    while (dataBuff[0] != '.') {
        pduLen = safeRecvfrom(socketNum, dataBuff, MAXBUF, 0, (struct sockaddr *) &client, &clientAddrLen);

        printf("Received message from client with ");
        printIPInfo(&client);

        printPDU(dataBuff, pduLen);

        // just for fun send back to client number of bytes received
        sprintf((char *) dataBuff, "bytes: %d", pduLen);
        safeSendto(socketNum, dataBuff, (int) strlen((char *) dataBuff) + 1, 0, (struct sockaddr *) &client, clientAddrLen);

        printf("\n");
    }
}

void checkArgs(int argc, char *argv[], int *port, float *errRate) {
    // Checks args and returns port number

    /* User can only give 1 or 2 arguments */
    if (argc < 2 || argc > 3) {
        fprintf(stderr, "\nusage: ./server error-rate [port-number]\n");
        exit(EXIT_FAILURE);
    }

    /* Get the error rate */
    *errRate = strtof(argv[1], NULL);

    /* Get the port */
    if (argc == 3) *port = (int) strtol(argv[2], NULL, 10);
    else *port = 0;

}


