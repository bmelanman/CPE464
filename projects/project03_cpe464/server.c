
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

    int ret;

    socklen_t serverAddrLen = sizeof(struct sockaddr_in6);
    struct sockaddr_in6 *serverAddr = scalloc(1, serverAddrLen);

    /* Make a socket */
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

    /* Set up the socket */
    serverAddr->sin6_family = AF_INET6;   /* IPv4/6 family       */
    serverAddr->sin6_addr = in6addr_any;  /* Use any IP          */
    serverAddr->sin6_port = htons(port);  /* Set/Request a port  */

    /* Bind the socket to a port and assign it an address */
    ret = bind(sock, (const struct sockaddr *) serverAddr, serverAddrLen);

    /* Error checking */
    if (ret != 0) {
        perror("bind() call error");
        exit(EXIT_FAILURE);
    }

    /* Get the socket name */
    ret = getsockname(sock, (struct sockaddr *) serverAddr, (socklen_t *) &serverAddrLen);

    /* Error checking */
    if (ret != 0) {
        perror("bind() call error");
        exit(EXIT_FAILURE);
    }

    /* Prompt */
    if (port == 0) {
        printf("Process started using port %d\n", ntohs(serverAddr->sin6_port));
    }

    return sock;
}

FILE *recvSetupInfo(int childSocket, pollSet_t *pollSet, addrInfo_t *clientInfo, uint16_t *bufferLen, circularQueue_t **packetQueue) {

    uint16_t maxFileLen = 100, count = 0;
    size_t pktLen;
    uint32_t windSize;
    char to_filename[maxFileLen];

    packet_t *rxPacket = initPacket();
    packet_t *txPacket = initPacket();
    FILE *fd = NULL;

    /** Connect the child process to the client **/

    /* Send the packet and wait for an Ack */
    while (1) {

        /* Check for timeout */
        if (count > 9) {
            printf("Child process could not reach client\n");
            return NULL;
        }

        /* Make an Ack packet */
        pktLen = buildPacket(txPacket, NO_PAYLOAD, SETUP_ACK_PKT, NULL, 0);

        /* Send packet */
        safeSendTo(childSocket, txPacket, pktLen, clientInfo);

        /* Check for a response  */
        if (pollCall(pollSet, POLL_1_SEC) != POLL_TIMEOUT) {

            /* Receive the response */
            pktLen = safeRecvFrom(childSocket, rxPacket, MAX_PDU_LEN, clientInfo);

            /* Verify the checksum and packet flag */
            if (in_cksum((unsigned short *) rxPacket, (int) pktLen) == 0 && rxPacket->flag == INFO_PKT) {

                /* Get the values from the payload */
                memcpy(bufferLen, (uint16_t *) &rxPacket->payload[HS_IDX_BUFF_LEN], sizeof(uint16_t));
                memcpy(&windSize, (uint32_t *) &rxPacket->payload[HS_IDX_WIND_LEN], sizeof(uint32_t));
                snprintf(to_filename, 100, "%s", &rxPacket->payload[HS_IDX_FILENAME]);

                /* Initialize the queue */
                (*packetQueue) = createQueue(windSize);

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

    /* Error checking */
    if (fd == NULL) {

        /* The client did not successfully send the file name, or timed out */
        printf("An error has occurred, connection terminated. \n");

        /* Make an ERR packet */
        pktLen = buildPacket(txPacket, 0, INFO_ERR_PKT, NULL, 0);

        /* Send packet */
        safeSendTo(childSocket, txPacket, pktLen, clientInfo);

        free(rxPacket);
        free(txPacket);

        return NULL;
    }

    /* Modify the new file's permissions */
    fchmodat(AT_FDCWD, to_filename, S_IRWXU | S_IRWXG | S_IRWXO, 0);

    /* Make an ACK packet */
    pktLen = buildPacket(txPacket, 0, INFO_ACK_PKT, NULL, 0);

    /* ACK and wait for data to start coming */
    while (1) {

        /* Check for disconnection */
        if (count > 9) {
            return NULL;
        }

        /* Send packet */
        safeSendTo(childSocket, txPacket, pktLen, clientInfo);

        /* Wait for client to reply */
        if (pollCall(pollSet, POLL_1_SEC) != POLL_TIMEOUT) {

            /* Receive packet */
            pktLen = safeRecvFrom(childSocket, rxPacket, (*bufferLen) + PDU_HEADER_LEN, clientInfo);

            /* Check for response data */
            if (rxPacket->flag == DATA_PKT || rxPacket->flag == DATA_EOF_PKT) {

                /* Save the packet for later */
                addQueuePacket(*packetQueue, rxPacket, pktLen);

                break;
            }
        } else {
            count++;
        }
    }

    free(rxPacket);
    free(txPacket);

    return fd;

}

void teardown(addrInfo_t *addrInfo, circularQueue_t *queue, pollSet_t *pollSet) {

    /* Teardown/Clean up */
    freeAddrInfo(addrInfo);

    freePollSet(pollSet);

    freeQueue(queue);

    // DYLD_INSERT_LIBRARIES=/usr/lib/libgmalloc.dylib
}

int runServer(int childSocket, addrInfo_t *clientInfo) {

    uint8_t count = 0;
    uint16_t bufferLen, pktLen;
    uint32_t serverSeq = 0, nextPkt = 0, nextPkt_NO = 0;

    FILE *new_fd = NULL;
    packet_t *packet = NULL, *txPacket = NULL;
    circularQueue_t *packetQueue = NULL;

    pollSet_t *pollSet = initPollSet();
    addToPollSet(pollSet, childSocket);

    /* Receive setup info from the client */
    new_fd = recvSetupInfo(childSocket, pollSet, clientInfo, &bufferLen, &packetQueue);

    /* Check for successful connection */
    if (new_fd == NULL) {
        teardown(clientInfo, packetQueue, pollSet);
        return 1;
    }

    /* Initialize the data packet struct */
    packet = initPacket();

    /* Receive file data from client */
    while (1) {

        /* Check if the output of the queue has the packet we want */
        if (checkNextSeq_NO(packetQueue, nextPkt_NO)) {

            /* Grab the packet from the queue, and skip checksum as it has already been verified */
            pktLen = getQueuePacket(packetQueue, packet);

        } else {

            /* Check client connection */
            if (pollCall(pollSet, POLL_10_SEC) == POLL_TIMEOUT) {
                teardown(clientInfo, packetQueue, pollSet);
                free(packet);
                fclose(new_fd);
                return 1;
            }

            /* Get the packet */
            pktLen = safeRecvFrom(childSocket, packet, bufferLen + PDU_HEADER_LEN, clientInfo);

            /* Validate checksum and packet sequence */
            if (in_cksum((unsigned short *) packet, pktLen)) {

                /* Selective reject the corrupt packet */
                buildPacket(packet, serverSeq++, DATA_REJ_PKT, (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));

            } else if (ntohl(packet->seq_NO) < nextPkt) {

                /* If it's a lower sequence, reply with an RR for the highest recv'd packet */
                nextPkt_NO = htonl(nextPkt - 1);

                buildPacket(packet, serverSeq++, DATA_ACK_PKT, (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));

            } else if (ntohl(packet->seq_NO) > nextPkt) {

                /* If it's a higher sequence, save it and send a SRej */
                addQueuePacket(packetQueue, packet, pktLen);

                /* We missed a packet, so we'll fill the queue until we have
                 * all the extra packets before we send a SRej */
                while (pollCall(pollSet, POLL_NON_BLOCK) != POLL_TIMEOUT) {

                    /* Get the packet */
                    pktLen = safeRecvFrom(childSocket, packet, bufferLen + PDU_HEADER_LEN, clientInfo);

                    /* If there's an invalid checksum, we'll just ignore it */
                    if (in_cksum((unsigned short *) packet, pktLen)) continue;

                    /* Make sure the received packet seq is larger than the last */
                    if (ntohl(packet->seq_NO) < peekLastSeq(packetQueue)) continue;

                    /* Save for later */
                    addQueuePacket(packetQueue, packet, pktLen);
                }

                /* SRej the missing packet */
                buildPacket(packet, serverSeq++, DATA_REJ_PKT, (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));
            }
        }

        /* Write data and make an RR */
        if (packet->flag == DATA_PKT) {

            /* Write the data to the file */
            fwrite(packet->payload, sizeof(uint8_t), pktLen - PDU_HEADER_LEN, new_fd);

            /* Create an ACK packet */
            buildPacket(packet, serverSeq++, DATA_ACK_PKT, (uint8_t *) &nextPkt_NO, sizeof(nextPkt_NO));

            /* Increment next expected packet sequence */
            nextPkt++;
        }

        /* Check for EOF */
        if (packet->flag == DATA_EOF_PKT) {

            /* Write the last of the data to the file */
            fwrite(packet->payload, sizeof(uint8_t), pktLen - PDU_HEADER_LEN, new_fd);

            /* Wrap things up */
            break;
        }

        /* Update */
        nextPkt_NO = htonl(nextPkt);

        /* Don't ACK if the next packet is in the queue */
        if (!checkNextSeq_NO(packetQueue, nextPkt_NO)) {

            /* Send the ACK/SRej/RR */
            safeSendTo(childSocket, packet, bufferLen + PDU_HEADER_LEN, clientInfo);

        }
    }

    /* Close new file */
    fclose(new_fd);

    printf("File transfer has been successfully received!\n");

    txPacket = initPacket();

    /* Ack the EOF packet by sending a termination request */
    pktLen = buildPacket(txPacket, serverSeq, TERM_CONN_PKT, NULL, 0);

    while (1) {

        /* Check for disconnection */
        if (count > 9) {
            teardown(clientInfo, packetQueue, pollSet);
            free(packet);
            fclose(new_fd);
            return 1;
        }

        /* Send packet */
        safeSendTo(childSocket, txPacket, pktLen, clientInfo);

        /* Wait for client to reply */
        if (pollCall(pollSet, POLL_1_SEC) != POLL_TIMEOUT) {

            /* Receive packet */
            safeRecvFrom(childSocket, packet, bufferLen + PDU_HEADER_LEN, clientInfo);

            /* Check for termination ACK */
            if (packet->flag == TERM_ACK_PKT) {
                break;
            }

        } else {
            count++;
        }
    }

    /* Clean up */
    teardown(clientInfo, packetQueue, pollSet);
    free(packet);
    free(txPacket);

    return 0;
}

void runServerController(int port, float errorRate) {

    int pollSock, childSock, stat, numChildren = 0, i;

    pollSet_t *pollSet = initPollSet();
    packet_t *setupPacket = initPacket();

    addrInfo_t *clientInfo = NULL;
    pid_t pid, *children = (pid_t *) scalloc(numChildren + 1, sizeof(pid_t));

    /* Set up a parent socket */
    addToPollSet(pollSet, udpServerSetup(port));

    /* Initialize sendErr() */
    sendErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_ON);

    printf("Server controller is initialized...\n");

    /* Run server */
    while (!shutdownServer) {

        /* Wait for a client to connect */
        pollSock = pollCall(pollSet, POLL_BLOCK);

        printf("Network socket activity detected...\n");

        /* Check poll for connections */
        if (pollSock != POLL_TIMEOUT) {

            /* Set up a new info struct */
            clientInfo = initAddrInfo();

            /* Grab the packet to get the client info for the child process */
            safeRecvFrom(pollSock, setupPacket, PDU_HEADER_LEN, clientInfo);

            /* Ignore packets that aren't setup packets */
            if (setupPacket->flag != SETUP_PKT) continue;

            /* Create a child to complete the transfer */
            pid = fork();

            /* Error checking */
            if (pid < 0) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            /* Split parent and child */
            if (pid == CHILD_PROCESS) {

                printf("Starting file transfer process...\n");

                /* Init sendToErr for the child process */
                sendErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_ON);

                /* Make a new socket for the child process */
                childSock = udpServerSetup(0);

                /* Run the child server to start the transfer */
                stat = runServer(childSock, clientInfo);

                /* Success/failure prompts */
                if (stat != 0) printf("Client has disconnected! \n");

                /* Terminate child process */
                exit(EXIT_SUCCESS);

            } else {

                /* Add the child to the list of child PIDs */
                children[numChildren] = pid;

                /* Increase the list sizes */
                numChildren++;
                children = srealloc(children, sizeof(pid_t) * numChildren);

            }
        }
    }

    /* Close children */
    for (i = 0; i < numChildren; ++i) {
        kill(children[i], SIGKILL);
    }

    /* Clean up */
    free(children);
    free(setupPacket);
    freePollSet(pollSet);

    printf("Clean exit!\n");

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


