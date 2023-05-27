
/* Simple UDP Server
 * Created by Bryce Melander in May 2023
 * using modified code from Dr. Hugh Smith
 *  */

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
    struct sockaddr_in6 serverAddress = {0};
    int socketNum;
    int serverAddrLen = 0;

    // create the socket
    if ((socketNum = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        perror("socket() call error");
        exit(-1);
    }

    // set up the socket
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

FILE *recvSetupInfo(pollSet_t *pollSet, addrInfo_t *clientInfo, uint16_t *bufferLen, circularQueue_t **packetQueue) {

    int pollSock, count = 0;
    uint16_t maxFileLen = 100, pktLen = maxFileLen + sizeof(uint16_t) + sizeof(uint32_t);
    uint32_t windSize;

    char to_filename[maxFileLen];
    packet_t *packet = initPacket(pktLen);
    FILE *fd = NULL;

    printf("Client connection request detected, initializing connection...\n");

    while (1) {

        if (count > 9) break;

        /* Check for incoming data */
        pollSock = pollCall(pollSet, POLL_1_SEC);

        /* Check for poll timeout */
        if (pollSock < 0) {
            count++;
            continue;
        }

        /* Get new message */
        pktLen = safeRecvFrom(pollSock, packet, MAX_PAYLOAD_LEN, clientInfo);

        /* Verify checksum and packet type */
        if (in_cksum((unsigned short *) packet, pktLen) == 0 && packet->flag == INFO_PKT) {

            /* Get the values from the payload */
            memcpy(bufferLen, (uint16_t *) &packet->payload[HS_IDX_BUFF_LEN], sizeof(uint16_t));
            memcpy(&windSize, (uint32_t *) &packet->payload[HS_IDX_WIND_LEN], sizeof(uint32_t));
            snprintf(to_filename, 100, "%s", &packet->payload[HS_IDX_FILENAME]);

            /* Initialize the queue */
            *packetQueue = createQueue(windSize, *bufferLen);

            /* Initialize the file pointer */
            fd = fopen(to_filename, "wb+");

            /* Leave the loop */
            break;
        }
    }

    /* Error checking */
    if (fd == NULL) {

        /* Prompt */
        printf("An error has occurred, connection terminated. \n");

        /* Make an ERR packet */
        pktLen = buildPacket(packet, *bufferLen, 0, INFO_ERR_PKT, NULL, 0);

    } else {

        /* Prompt */
        printf(
                "Setup Complete!        \n"
                "\tBuffer Size: %d      \n"
                "\tWindow Size: %d      \n"
                "\tto-filename: %s      \n"
                "\n",
                *bufferLen, windSize, to_filename
        );

        /* Make an ACK packet */
        pktLen = buildPacket(packet, *bufferLen, 0, INFO_ACK_PKT, NULL, 0);
    }

    /* Send packet */
    safeSendTo(pollSock, packet, pktLen, clientInfo);

    return fd;

}

void teardown(addrInfo_t *addrInfo, FILE *fd, circularQueue_t *queue, packet_t *packet) {

    freeAddrInfo(addrInfo);

    fclose(fd);

    freePacket(packet);

    freeQueue(queue);

}

int runServer(int serverSocket, pollSet_t *pollSet) {

    uint8_t count = 0;
    uint16_t bufferLen, pduLen;
    uint32_t serverSeq = 0, nextPkt = 0, nextPkt_NO = 0;

    addrInfo_t *client = initAddrInfo();
    FILE *newFile = NULL;
    circularQueue_t *packetQueue = NULL;
    packet_t *packet = NULL;

    /* TODO: Free + fclose */

    /* Receive setup info from the client */
    newFile = recvSetupInfo(pollSet, client, &bufferLen, &packetQueue);

    /* Initialize the data packet struct */
    packet = initPacket(bufferLen);

    /* Check for successful connection */
    if (newFile == NULL) {
        teardown(client, newFile, packetQueue, packet);
        return 1;
    }

    printf("Waiting for data from client...\n");

    /* Receive file data from client */
    while (1) {

        /* Check if the output of the queue has the packet we want */
        if (peekNextSeq_NO(packetQueue, nextPkt_NO)) {

            /* Grab the packet from the queue, and skip checksum as it has already been verified */
            pduLen = getQueuePacket(packetQueue, packet);

        } else {

            /* Check client connection */
            if (pollCall(pollSet, POLL_10_SEC) == POLL_TIMEOUT) {
                teardown(client, newFile, packetQueue, packet);
                return 1;
            }

            /* Get the packet */
            pduLen = safeRecvFrom(serverSocket, packet, bufferLen + PDU_HEADER_LEN, client);

            /* Validate checksum and packet sequence */
            if (in_cksum((unsigned short *) packet, pduLen)) {

                /* Selective reject the corrupt packet */
                buildPacket(packet, bufferLen, serverSeq++, DATA_REJ_PKT, (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));

            } else if (ntohl(packet->seq_NO) < nextPkt) {

                /* If it's a lower sequence, reply with an RR for the highest recv'd packet */
                nextPkt_NO = htonl(nextPkt - 1);

                buildPacket(packet, bufferLen, serverSeq++, DATA_ACK_PKT, (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));

            } else if (ntohl(packet->seq_NO) > nextPkt) {
                /* If it's a higher sequence, save it and send a SRej */
                addQueuePacket(packetQueue, packet, pduLen);

                /* SRej the missing packet */
                buildPacket(packet, bufferLen, serverSeq++, DATA_REJ_PKT, (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));
            }
        }

        /* Write data and make an RR */
        if (packet->flag == DATA_PKT) {

            /* Write the data to the file */
            fwrite(packet->payload, sizeof(uint8_t), pduLen - PDU_HEADER_LEN, newFile);

            /* Create an ACK packet */
            buildPacket(packet, bufferLen, serverSeq++, DATA_ACK_PKT, (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));

            /* Increment next expected packet sequence */
            nextPkt++;
        }

        /* Check for EOF */
        if (packet->flag == DATA_EOF_PKT) {

            /* Write the last of the data to the file */
            fwrite(packet->payload, sizeof(uint8_t), pduLen - PDU_HEADER_LEN, newFile);

            /* Wrap things up */
            break;
        }

        /* Update */
        nextPkt_NO = htonl(nextPkt);

        /* Don't ACK if the next packet is in the queue */
        if (!peekNextSeq_NO(packetQueue, nextPkt_NO)) {

            /* Send the ACK/SRej/RR */
            safeSendTo(serverSocket, packet, bufferLen + PDU_HEADER_LEN, client);

        }
    }

    /* Close new file */
    fclose(newFile);

    /* Ack the EOF packet by sending a termination request */
    pduLen = buildPacket(packet, bufferLen, serverSeq, TERM_CONN_PKT, NULL, 0);

    /* Add the packet to the queue in case we need to re-send it */
    addQueuePacket(packetQueue, packet, pduLen);

    while (1) {

        /* Check for disconnection */
        if (count > 9)  {
            teardown(client, newFile, packetQueue, packet);
            return 1;
        }

        /* Get the termination packet */
        packet = peekQueuePacket(packetQueue);

        /* Send packet */
        safeSendTo(serverSocket, packet, pduLen, client);

        /* Wait for client to reply */
        if (pollCall(pollSet, POLL_1_SEC) != POLL_TIMEOUT) {

            /* Receive packet */
            safeRecvFrom(serverSocket, packet, pduLen, client);

            /* Check for termination ACK */
            if (packet->flag == TERM_ACK_PKT) break;
        } else {
            count++;
        }
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

    freePollSet(pollSet);
}

void checkArgs(int argc, char *argv[], float *errRate, int *port) {

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


