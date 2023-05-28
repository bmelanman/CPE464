
/* Simple UDP Server
 * Created by Bryce Melander in May 2023
 * using modified code from Dr. Hugh Smith
 *  */

#include <signal.h>
#include "server.h"

static volatile int shutdownServer = 0;

void intHandler(void) {
    printf("\n\nShutting down server...\n");
    shutdownServer = 1;
}

void checkArgs(int argc, char *argv[], float *errRate, int *port);

int udpServerSetup(int port);

void runServerController(int port, float errorRate);

int main(int argc, char *argv[]) {
    int port;
    float errorRate;

    /* Catch for ^C */
    signal(SIGINT, (void (*)(int)) intHandler);

    /* Verify user input */
    checkArgs(argc, argv, &errorRate, &port);

    /* Connect to a client */
    runServerController(port, errorRate);

    return 0;
}

/*!
 * Creates a UDP socket on the server side and binds to that socket. \n\n
 * Written by Hugh Smith – April 2017
 * @param port - The port to host the new server socket
 * @return The server's socket
 */
int udpServerSetup(int port) {

    int ret, serverAddrLen = sizeof(struct sockaddr_in6);
    struct sockaddr_in6 *serverAddr = malloc(serverAddrLen);

    // create the socket
    int sock = socket(AF_INET6, SOCK_DGRAM, 0);

    /* Error checking */
    if (sock < 0) {
        perror("socket() call error");
        exit(EXIT_FAILURE);
    }

    /* Reuse address for easier debugging */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    // set up the socket
    serverAddr->sin6_family = AF_INET6;   // internet (IPv6 or IPv4) family
    serverAddr->sin6_addr = in6addr_any;  // use any local IP address
    serverAddr->sin6_port = htons(port);  // if 0 = os picks

    // bind the name (address) to a port
    ret = bind(sock, (struct sockaddr *) serverAddr, serverAddrLen);

    /* Error checking */
    if (ret < 0) {
        perror("bind() call error");
        exit(EXIT_FAILURE);
    }

    /* Get the port number */
    getsockname(sock, (struct sockaddr *) serverAddr, (socklen_t *) &serverAddrLen);
    printf("Server using Port #: %d\n", ntohs(serverAddr->sin6_port));

    return sock;
}

FILE *recvSetupInfo(int parentSocket, int childSocket, pollSet_t *pollSet, uint16_t *bufferLen, addrInfo_t *clientInfo,
                    circularQueue_t **packetQueue) {

    int pollSock, count = 0;
    uint16_t maxFileLen = 100;
    size_t pktLen = maxFileLen + sizeof(uint16_t) + sizeof(uint32_t);
    uint32_t windSize;

    char to_filename[maxFileLen];
    packet_t *packet = initPacket(pktLen);
    FILE *fd = NULL;

    /** Connect the child process to the client **/
    printf("Client connection request detected, initializing connection...\n");

    /* Add both sockets to the poll set */
    addToPollSet(pollSet, parentSocket);
    addToPollSet(pollSet, childSocket);

    printf("Parent socket: %d\n"
           "Child socket: %d\n",
           parentSocket, childSocket
    );

    /* Send the packet and wait for an Ack */
    while (1) {

        /* Check for timeout */
        if (count > 9) {
            printf("Child process could not reach client\n");
            return NULL;
        }

        /* Check for activity */
        pollSock = pollCall(pollSet, POLL_1_SEC);

        if (pollSock == parentSocket) {

            printf("Parent socket\n");

            /* Receive initial the client message */
            pktLen = safeRecvFrom(parentSocket, packet, pktLen + PDU_HEADER_LEN, clientInfo);

            /* Verify the checksum and packet flag */
            if (in_cksum((unsigned short *) packet, (int) pktLen) == 0 && packet->flag == SETUP_PKT) {

                /* Make a setup Ack packet */
                pktLen = buildPacket(packet, 0, 0, SETUP_ACK_PKT, NULL, 0);

                /* Send the setup Ack packet */
                safeSendTo(childSocket, packet, pktLen, clientInfo);

            }

        } else if (pollSock == childSocket) {

            printf("Child socket\n");

            /* Receive the response */
            pktLen = safeRecvFrom(childSocket, packet, MAX_PDU_LEN, clientInfo);

            /* Verify the checksum and packet flag */
            if (in_cksum((unsigned short *) packet, (int) pktLen) == 0 && packet->flag == INFO_PKT) {

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

        } else {

            /* Increment timeout */
            count++;

        }
    }

    /* Remove the parent socket, only the child socket is needed now */
    removeFromPollSet(pollSet, parentSocket);

    /* Error checking */
    if (fd == NULL) {

        /* Prompt */
        printf("An error has occurred, connection terminated. \n");

        /* Make an ERR packet */
        pktLen = buildPacket(packet, *bufferLen, 0, INFO_ERR_PKT, NULL, 0);

        /* Send packet */
        safeSendTo(childSocket, packet, pktLen, clientInfo);

        return NULL;
    }

    printf("Child sending Ack\n");

    /* Make an ACK packet */
    pktLen = buildPacket(packet, *bufferLen, 0, INFO_ACK_PKT, NULL, 0);

    /* Save the packet */
    addQueuePacket(*packetQueue, packet, pktLen);

    /* ACK and wait for data to start coming */
    while (1) {

        /* Check for disconnection */
        if (count > 9) {
            return NULL;
        }

        /* Get the Ack packet */
        pktLen = peekQueuePacket(*packetQueue, packet);

        /* Send packet */
        safeSendTo(childSocket, packet, pktLen, clientInfo);

        /* Wait for client to reply */
        if (pollCall(pollSet, POLL_1_SEC) != POLL_TIMEOUT) {

            /* Receive packet */
            pktLen = safeRecvFrom(childSocket, packet, *bufferLen + PDU_HEADER_LEN, clientInfo);

            /* Check for response data */
            if (packet->flag == DATA_PKT) {

                /* Save the packet for later */
                addQueuePacket(*packetQueue, packet, pktLen);

                break;
            }
        } else {
            count++;
        }
    }

    /* Prompt */
    printf(
            "Setup Complete!        \n"
            "\tBuffer Size: %d      \n"
            "\tWindow Size: %d      \n"
            "\tto-filename: %s      \n"
            "\n",
            *bufferLen, windSize, to_filename
    );

    /* Clear the packet Ack from the queue */
    getQueuePacket(*packetQueue, packet);

    /* Clean up */
    free(packet);

    return fd;

}

void teardown(addrInfo_t *addrInfo, FILE *fd, circularQueue_t *queue, packet_t *packet) {

    freeAddrInfo(addrInfo);

    fclose(fd);

    free(packet);

    freeQueue(queue);

}

int runServer(int parentSocket, int childSocket) {

    uint8_t count = 0;
    uint16_t bufferLen, pduLen;
    uint32_t serverSeq = 0, nextPkt = 0, nextPkt_NO = 0;

    addrInfo_t *clientInfo = initAddrInfo();
    pollSet_t *pollSet = initPollSet();

    FILE *newFile = NULL;

    packet_t *packet = NULL;
    circularQueue_t *packetQueue = NULL;

    /* Receive setup info from the client */
    newFile = recvSetupInfo(parentSocket, childSocket, pollSet, &bufferLen, clientInfo, &packetQueue);

    /* Initialize the data packet struct */
    packet = initPacket(bufferLen);

    /* Check for successful connection */
    if (newFile == NULL) {
        teardown(clientInfo, newFile, packetQueue, packet);
        return 1;
    }

    printf("Receiving from client...\n");

    /* Receive file data from client */
    while (1) {

        /* Check if the output of the queue has the packet we want */
        if (peekNextSeq_NO(packetQueue, nextPkt_NO)) {

            /* Grab the packet from the queue, and skip checksum as it has already been verified */
            pduLen = getQueuePacket(packetQueue, packet);

        } else {

            /* Check client connection */
            if (pollCall(pollSet, POLL_10_SEC) == POLL_TIMEOUT) {
                teardown(clientInfo, newFile, packetQueue, packet);
                return 1;
            }

            /* Get the packet */
            pduLen = safeRecvFrom(childSocket, packet, bufferLen + PDU_HEADER_LEN, clientInfo);

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
            safeSendTo(childSocket, packet, bufferLen + PDU_HEADER_LEN, clientInfo);

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
        if (count > 9) {
            teardown(clientInfo, newFile, packetQueue, packet);
            return 1;
        }

        /* Get the termination packet */
        pduLen = peekQueuePacket(packetQueue, packet);

        /* Send packet */
        safeSendTo(childSocket, packet, pduLen, clientInfo);

        /* Wait for client to reply */
        if (pollCall(pollSet, POLL_1_SEC) != POLL_TIMEOUT) {

            /* Receive packet */
            safeRecvFrom(childSocket, packet, bufferLen + PDU_HEADER_LEN, clientInfo);

            /* Check for termination ACK */
            if (packet->flag == TERM_ACK_PKT) break;
        } else {
            count++;
        }
    }

    /* Clean up */
    teardown(clientInfo, newFile, packetQueue, packet);

    return 0;
}

void runServerController(int port, float errorRate) {

    int pollSock, childSock, stat, numChildren = 0;
    pid_t pid = 0, *children = malloc(sizeof(pid_t) * (numChildren + 1));
    pollSet_t *pollSet = initPollSet();

    /* Set up a parent socket */
    addToPollSet(pollSet, udpServerSetup(port));

    /* Initialize sendErr() */
    sendErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);

    while (!shutdownServer) {

        /* Wait for a client to connect */
        pollSock = pollCall(pollSet, POLL_BLOCK);

        if (pid != 0) continue; // TODO: REMOVE

        /* Check poll for connections */
        if (pollSock != POLL_TIMEOUT) {

            /* Create a child to complete the transfer */
            pid = fork();

            /* Error checking */
            if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            /* Split parent and child */
            if (pid == CHILD_PROCESS) {

                /* Init sendToErr for the child process */
                sendErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);

                /* Make a new socket for the child process */
                childSock = udpServerSetup(0);

                /* Run the child server to start the transfer */
                stat = runServer(pollSock, childSock);

                /* Success/failure prompts */
                if (stat != 0) printf("Client has disconnected! \n");
                else printf("File transfer has successfully completed!\n");

                /* Terminate child process */
                exit(EXIT_SUCCESS);

            } else {

                printf("\nChild forked with PID %d\n", pid);

                /* Add the child to the list of child PIDs */
                children[numChildren++] = pid;

                /* Increase the list size */
                children = srealloc(children, sizeof(pid_t) * (numChildren + 1));

                waitpid(pid, NULL, 0);

            }
        }
    }

    /* Close children */
    for (int i = 0; i < numChildren; ++i) {
        kill(children[i], SIGKILL);
    }

    free(children);

    freePollSet(pollSet);

    printf("Clean close!\n");
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


