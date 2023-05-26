
/* Simple UDP Server
 * Created by Bryce Melander in May 2023
 * using modified code from Dr. Hugh Smith
 *  */


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "networkUtils.h"
#include "pollLib.h"
#include "windowLib.h"

#include "server.h"

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

uint8_t *recvSetupInfo(pollSet_t *pollSet, addrInfo_t *clientInfo) {

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
        pduLen = safeRecvFrom(pollSock, &dataPDU, MAX_PDU_LEN, clientInfo);

        /* Verify checksum */
        if (in_cksum((unsigned short *) &dataPDU, pduLen) != 0) continue;

        /* Verify correct packet type */
        if (dataPDU.flag != INFO_PKT) {
            count++;
            continue;
        }

        /* Save the payload info */
        recvPayload = malloc(pduLen - PDU_HEADER_LEN + 1);
        memcpy(recvPayload, dataPDU.payload, pduLen - PDU_HEADER_LEN);

        /* Send an ACK after receiving */
        createPDU(&dataPDU, 0, INFO_ACK_PKT, (uint8_t *) &seq, sizeof(uint16_t));

        safeSendTo(pollSock, (void *) &dataPDU, pduLen, clientInfo);

        /* TODO: Wait for data packet first? Then maybe put it in the queue? Or maybe do the ACK in runServer() */

        return recvPayload;
    }
}

FILE *getTransferInfo(uint8_t *data, uint16_t *bufferLen, circularQueue_t **queue) {

    FILE *fd = NULL;

    /* Get the values from the payload */
    *bufferLen = *((uint16_t *) data) + PDU_HEADER_LEN;
    uint32_t winSize = *((uint32_t *) &data[2]);
    char *to_filename = strndup((char *) &data[6], 100);

    /* Initialize the queue */
    *queue = createQueue(winSize);

    /* Check for invalid data */
    if (to_filename == NULL) return NULL;

    fd = fopen(to_filename, "wb+");

    printf(
            "Setup Complete!        \n"
            "\tBuffer Size: %d      \n"
            "\tWindow Size: %d      \n"
            "\tto-filename: %s      \n"
            "\n",
            *bufferLen - PDU_HEADER_LEN, winSize, to_filename
    );

    return fd;
}

//int sendMessagePacket

int runServer(int serverSocket, pollSet_t *pollSet) {

    uint8_t *transferInfo, count = 0;
    uint16_t bufferLen, pduLen;
    uint32_t serverSeq = 0, nextPkt = 0, nextPkt_NO = 0;

    addrInfo_t *client;
    FILE *newFile = NULL;
    circularQueue_t *packetQueue = NULL;
    udpPacket_t dataPDU = {0};

    /* TODO: Free + fclose */

    /* Receive setup info from the client */
    transferInfo = recvSetupInfo(pollSet, client);

    /* Check for disconnection */
    if (transferInfo == NULL) return 1;

    /* Set up with transfer data */
    newFile = getTransferInfo(transferInfo, &bufferLen, &packetQueue);

    /* Make sure the file opened */
    /* TODO: SEND ERROR MESSAGE */
    if (newFile == NULL) return 1;

    printf("Waiting for data from client...\n");

    /* Receive file data from client */
    while (1) {

        /* Check if the output of the queue has the packet we want */
        if (peekNextSeq_NO(packetQueue, nextPkt_NO)) {

            /* Grab the packet from the queue, and skip checksum as it has already been verified */
            pduLen = getQueuePacket(packetQueue, &dataPDU);

        } else {

            /* Check client connection */
            if (pollCall(pollSet, POLL_10_SEC) == POLL_TIMEOUT) return 1;

            /* Get the packet */
            pduLen = safeRecvFrom(serverSocket, &dataPDU, bufferLen, (struct sockaddr *) &client);

            /* Validate checksum and packet sequence */
            if (in_cksum((unsigned short *) &dataPDU, pduLen)) {

                /* Selective reject the corrupt packet */
                createPDU(&dataPDU, serverSeq++, DATA_REJ_PKT, (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));

            } else if (ntohl(dataPDU.seq_NO) < nextPkt) {

                /* If it's a lower sequence, reply with an RR for the highest recv'd packet */
                nextPkt_NO = htonl(nextPkt - 1);

                createPDU(&dataPDU, serverSeq++, DATA_ACK_PKT, (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));

            } else if (ntohl(dataPDU.seq_NO) > nextPkt) {
                /* If it's a higher sequence, save it and send a SRej */
                addQueuePacket(packetQueue, &dataPDU, pduLen);

                /* SRej the missing packet */
                createPDU(&dataPDU, serverSeq++, DATA_REJ_PKT, (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));
            }
        }

        /* Write data and make an RR */
        if (dataPDU.flag == DATA_PKT) {

            /* Write the data to the file */
            fwrite(dataPDU.payload, sizeof(uint8_t), pduLen - PDU_HEADER_LEN, newFile);

            /* Create an ACK packet */
            createPDU(&dataPDU, serverSeq++, DATA_ACK_PKT, (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));

            /* Increment next expected packet sequence */
            nextPkt++;
        }

        /* Check for EOF */
        if (dataPDU.flag == DATA_EOF_PKT) {

            /* Write the last of the data to the file */
            fwrite(dataPDU.payload, sizeof(uint8_t), pduLen - PDU_HEADER_LEN, newFile);

            /* Wrap things up */
            break;
        }

        /* Update */
        nextPkt_NO = htonl(nextPkt);

        /* Don't ACK if the next packet is in the queue */
        if (!peekNextSeq_NO(packetQueue, nextPkt_NO)) {

            /* Send the ACK/SRej/RR */
            safeSendTo(serverSocket, &dataPDU, bufferLen, (struct sockaddr *) &client);

        }
    }

    /* Close new file */
    fclose(newFile);

    /* Ack the EOF packet by sending a termination request */
    createPDU(&dataPDU, serverSeq, TERM_CONN_PKT, NULL, 0);

    /* Add the packet to the queue in case we need to re-send it */
    addQueuePacket(packetQueue, &dataPDU, PDU_HEADER_LEN);

    while (1) {

        /* Check for disconnection */
        if (count > 9) return 1;

        /* Get the termination packet */
        dataPDU = *peekQueuePacket(packetQueue);

        /* Send packet */
        safeSendTo((int) nextPkt_NO, &dataPDU, bufferLen, (struct sockaddr *) &client);

        /* Wait for client to reply */
        if (pollCall(pollSet, POLL_1_SEC) != POLL_TIMEOUT) {

            /* Receive packet */
            safeRecvFrom(serverSocket, &dataPDU, bufferLen, (struct sockaddr *) &client);

            /* Check for termination ACK */
            if (dataPDU.flag == TERM_ACK_PKT) break;
        } else count++;
    }

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
            stat = runServer(serverSocket, pollSet);

            if (stat != 0) printf("Client has disconnected! \n");
            else printf("File transfer has successfully completed!\n");
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


