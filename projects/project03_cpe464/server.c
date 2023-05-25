
/* Simple UDP Server
 * Created by Bryce Melander in May 2023
 * using modified code from Dr. Hugh Smith
 *  */

#include <sys/file.h>
#include "server.h"
#include "libPoll.h"
#include "windowLib.h"

static volatile int shutdownServer = 0;

void intHandler(void) {
    printf("\n\nShutting down server...\n");
    shutdownServer = 1;
}

void checkArgs(int argc, char *argv[], float *errRate, int *port);

int udpServerSetup(int serverPort);

void runServerController(int serverSocket);

int main(int argc, char *argv[]) {
    int socket, port;
    float errorRate;

    /* Catch for ^C */
    signal(SIGINT, (void (*)(int)) intHandler);

    /* Verify user input */
    checkArgs(argc, argv, &errorRate, &port);

    /* Set up UDP */
    socket = udpServerSetup(port);

    /* Initialize sendErr() */
    sendErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);

    /* Connect to a client */
    runServerController(socket);

    /* Clean up */
    close(socket);

    return 0;
}

/*!
 * Creates a UDP socket on the server side and binds to that socket. \n\n
 * Written by Hugh Smith – April 2017
 * @param serverPort
 * @return The server's socket
 */
int udpServerSetup(int serverPort) {
    struct sockaddr_in6 serverAddress;
    int socketNum;
    int serverAddrLen = 0;

    // create the socket
    if ((socketNum = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        perror("socket() call error");
        exit(-1);
    }

    // set up the socket
    memset(&serverAddress, 0, sizeof(struct sockaddr_in6));
    serverAddress.sin6_family = AF_INET6;            // internet (IPv6 or IPv4) family
    serverAddress.sin6_addr = in6addr_any;        // use any local IP address
    serverAddress.sin6_port = htons(serverPort);   // if 0 = os picks

    // bind the name (address) to a port
    if (bind(socketNum, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        perror("bind() call error");
        exit(-1);
    }

    /* Get the port number */
    serverAddrLen = sizeof(serverAddress);
    getsockname(socketNum, (struct sockaddr *) &serverAddress, (socklen_t *) &serverAddrLen);
    printf("Server using Port #: %d\n", ntohs(serverAddress.sin6_port));

    return socketNum;

}

uint8_t *recvSetupInfo(pollSet_t *pollSet, struct sockaddr_in6 *client, int clientAddrLen) {

    int pollSock, count = 0;
    uint8_t *recvPayload;
    uint16_t seq = 0, pduLen;
    udpPacket_t dataPDU = {0};

    while (1) {

        if (count > 10) return NULL;
        else count++;

        pollSock = pollCall(pollSet, POLL_1_SEC);

        if (pollSock <= 0) continue;

        /* Receive the connection info packet */
        pduLen = safeRecvFrom(pollSock, &dataPDU, MAX_PDU_LEN, 0,
                              (struct sockaddr *) client, clientAddrLen);

        /* Verify checksum */
        if (in_cksum((unsigned short *) &dataPDU, pduLen) != 0) continue;

        /* Verify correct packet type */
        if (dataPDU.flag != INFO_PKT) continue;

        /* Save the payload info */
        recvPayload = malloc(pduLen - PDU_HEADER_LEN + 1);
        memcpy(recvPayload, dataPDU.payload, pduLen - PDU_HEADER_LEN);

        /* Send an ACK after receiving */
        createPDU(&dataPDU, 0, INFO_ACK_PKT, (uint8_t *) &seq, sizeof(uint16_t));
        safeSendTo(pollSock, (void *) &dataPDU, pduLen,
                   (struct sockaddr *) client, clientAddrLen);

        return recvPayload;
    }
}

char *getTransferInfo(uint8_t *data, uint16_t *bufferLen, uint32_t *windowSize) {

    *bufferLen = *((uint16_t *) data);
    *windowSize = *((uint32_t *) &data[2]);
    char *to_filename = strndup((char *) &data[6], 100);

    printf(
            "Setup Complete!        \n"
            "\tBuffer Size: %d      \n"
            "\tWindow Size: %d      \n"
            "\tto-filename: %s      \n"
            "\n",
            *bufferLen, *windowSize, to_filename
    );

    return to_filename;
}

int runServer(pollSet_t *pollSet) {

    int pollSock;
    uint8_t *transferInfo, count = 0, sentRej = 0;
    uint16_t bufferLen, pduLen;
    uint32_t windowSize, txSeq = 0, nextPkt = 0, nextPkt_NO;
    char *to_filename;
    FILE *newFile = NULL;
    udpPacket_t dataPDU = {0};
    int clientAddrLen = sizeof(struct sockaddr_in6);
    struct sockaddr_in6 client;
    circularQueue_t *packetQueue = NULL;

    /* TODO: Free + fclose */

    /* Receive setup info from the client */
    transferInfo = recvSetupInfo(pollSet, &client, clientAddrLen);
    to_filename = getTransferInfo(transferInfo, &bufferLen, &windowSize);

    /* Initialize the queue */
    packetQueue = createQueue(windowSize);

    /* Add the header to the buffer length */
    bufferLen += PDU_HEADER_LEN;

    /* Check for invalid data */
    if (transferInfo == NULL || to_filename == NULL) return 1;

    /* Open the new file */
    newFile = fopen(to_filename, "wb+");

    /* Make sure the file opened */
    if (newFile == NULL) return 1;

    printf("Waiting for data from client...\n");

    /* Receive file data from client */
    while (1) {

        nextPkt_NO = htonl(nextPkt);

        /* Check if the output of the queue has the packet we want */
        if (peekNextSeq_NO(packetQueue, nextPkt_NO)) {

            /* Grab the packet from the queue, and skip checksum as it has already been verified */
            pduLen = getQueuePacket(packetQueue, &dataPDU);

        } else {

            /* Wait for new data */
            pollSock = pollCall(pollSet, POLL_10_SEC);

            /* Check if client disconnected */
            if (pollSock == POLL_TIMEOUT) return 1;

            /* Get the packet */
            pduLen = safeRecvFrom(pollSock, &dataPDU, bufferLen, 0, (struct sockaddr *) &client, clientAddrLen);

            /* Validate checksum and packet sequence */
            if (in_cksum((unsigned short *) &dataPDU, pduLen)) {

                /* Selective reject the corrupt packet */
                createPDU(&dataPDU, txSeq++, DATA_REJ_PKT, (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));

            } else if (ntohl(dataPDU.seq_NO) < nextPkt) {

                /* If it's a lower sequence, reply with an RR for the highest recv'd packet */
                nextPkt_NO = htonl(nextPkt - 1);

                createPDU(&dataPDU, txSeq++, DATA_ACK_PKT, (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));

            } else if (ntohl(dataPDU.seq_NO) > nextPkt) {
                /* If it's a higher sequence, save it and send a SRej */
                addQueuePacket(packetQueue, &dataPDU, pduLen);

                /* SRej the missing packet */
                createPDU(&dataPDU, txSeq++, DATA_REJ_PKT, (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));
            }

        }

        /* Write data and RR */
        if (dataPDU.flag == DATA_PKT) {

            /* Write the data to the file */
            fwrite(dataPDU.payload, sizeof(uint8_t), pduLen - PDU_HEADER_LEN, newFile);

            /* Create an ACK packet */
            createPDU(&dataPDU, txSeq++, DATA_ACK_PKT, (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));

            /* Increment next expected packet sequence */
            nextPkt++;
        }

        /* Send the ACK/SRej/RR */
        safeSendTo(pollSock, &dataPDU, bufferLen, (struct sockaddr *) &client, clientAddrLen);

        if (dataPDU.flag == DATA_EOF_PKT) break;

//        /* Prioritize getting packets form the queue if possible */
//        if (peekNextSeq_NO(packetQueue, 0) == nextPkt) {
//
//            /* Grab a packet from the queue */
//            pduLen = getQueuePacket(packetQueue, &dataPDU);
//
//        } else if (sentRej && dataPDU.flag == DATA_ACK_PKT) {
//
//            /* Create an ACK packet */
//            createPDU(&dataPDU, txSeq++, DATA_ACK_PKT,
//                      (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));
//
//            /* If the buffer didn't have the next packet, send the RR */
//            safeSendTo(pollSock, &dataPDU, bufferLen, (struct sockaddr *) &client, clientAddrLen);
//
//            nextPkt++;
//            sentRej = 0;
//
//            continue;
//        } else {
//            /* Wait for new data */
//            pollSock = pollCall(pollSet, POLL_10_SEC);
//
//            /* Check if client disconnected */
//            if (pollSock == POLL_TIMEOUT) return 1;
//
//            /* Get the packet */
//            pduLen = safeRecvFrom(pollSock, &dataPDU, bufferLen, 0,
//                                  (struct sockaddr *) &client, clientAddrLen);
//        }
//
//        /* Validate checksum and packet sequence */
//        if (in_cksum((unsigned short *) &dataPDU, pduLen)) {
//
//            /* Selective reject the corrupt packet */
//            createPDU(&dataPDU, txSeq++, DATA_REJ_PKT,
//                      (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));
//
//            sentRej = 1;
//
//        } else if (ntohl(dataPDU.seq_NO) < nextPkt) {
//
//            /* If it's a lower sequence, reply with an RR for the highest recv'd packet */
//            nextPkt_NO = htonl(nextPkt - 1);
//
//            createPDU(&dataPDU, txSeq++, DATA_ACK_PKT,
//                      (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));
//
//        } else if (ntohl(dataPDU.seq_NO) > nextPkt) {
//            /* If it's a higher sequence, save it and send a SRej */
//            addQueuePacket(packetQueue, &dataPDU, pduLen);
//
//            /* SRej the missing packet */
//            createPDU(&dataPDU, txSeq++, DATA_REJ_PKT,
//                      (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));
//
//            sentRej = 1;
//
//        } else {
//
//            /* Write the data to the file */
//            fwrite(dataPDU.payload, sizeof(uint8_t), pduLen - PDU_HEADER_LEN, newFile);
//
//            /* Check for EOF flag */
//            if (dataPDU.flag == DATA_EOF_PKT) break;
//
//            /* Create an ACK packet */
//            createPDU(&dataPDU, txSeq++, DATA_ACK_PKT, (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));
//
//            /* Increment next expected packet sequence */
//            nextPkt++;
//
//            /* If a SRej was prev. sent, check the buffer before sending an RR */
//            if (sentRej) continue;
//        }
//
//        /* Send the packet */
//        safeSendTo(pollSock, &dataPDU, bufferLen, (struct sockaddr *) &client, clientAddrLen);

    }

    /* Close new file */
    fclose(newFile);

    /* Ack the EOF packet by sending a termination request */
    createPDU(&dataPDU, txSeq, TERM_CONN_PKT, NULL, 0);
    addQueuePacket(packetQueue, &dataPDU, PDU_HEADER_LEN);

    /* Copy the socket */
    nextPkt_NO = pollSock;

    while (1) {

        if (++count > 10) break;

        /* Send packet */
        safeSendTo((int) nextPkt_NO, &dataPDU, bufferLen, (struct sockaddr *) &client, clientAddrLen);

        /* Wait for client ACK */
        pollSock = pollCall(pollSet, POLL_1_SEC);

        if (pollSock != POLL_TIMEOUT) {

            safeRecvFrom(pollSock, &dataPDU, bufferLen, 0, (struct sockaddr *) &client, clientAddrLen);

            /* Check for termination ACK, otherwise restore the packet */
            if (dataPDU.flag == TERM_ACK_PKT) break;
            else dataPDU = *peekQueuePacket(packetQueue);
        }
    }

    printf("File transfer has successfully completed!\n");

    return 0;
}

void runServerController(int serverSocket) {

    int pollSocket, stat;
    pollSet_t *pollSet = newPollSet();

    addToPollSet(pollSet, serverSocket);

    while (!shutdownServer) {

        /* Wait for a client to connect */
        pollSocket = pollCall(pollSet, POLL_BLOCK);

        if (pollSocket == serverSocket) {
            /* Fork children here in the future */
            stat = runServer(pollSet);

            if (stat != 0) printf("Client has become disconnected!\n");
        }

    }

}

void checkArgs(int argc, char *argv[], float *errRate, int *port) {
    // Checks args and returns port number

    /* User can only give 1 or 2 arguments */
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "\nusage: ./server error-rate [port-number]\n");
        exit(EXIT_FAILURE);
    }

    /* Get the error rate */
    *errRate = strtof(argv[1], NULL);

    /* Check that error rate is in a valid range */
    if (*errRate >= 1.0 || *errRate < 0) {
        fprintf(stderr, "\nError: error-rate must be greater than or equal to 0, and less than 1\n");
        exit(EXIT_FAILURE);
    }

    /* Check if a port is provided */
    if (argc == 3) {
        *port = (int) strtol(argv[ARGV_PORT_NUM], NULL, 10);
    } else {
        *port = 0;
    }

}


