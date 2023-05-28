
/* Simple UDP Client
 * Created by Bryce Melander in May 2023
 * using modified code from Dr. Hugh Smith
 * */

#include "rcopy.h"

void checkArgs(int argc, char *argv[], FILE **fp, runtimeArgs_t *usrArgs);

int setupUdpClientToServer(addrInfo_t *serverInfo, char *hostName, int hostPort);

int setupTransfer(int socket, addrInfo_t *serverInfo, runtimeArgs_t *usrArgs, pollSet_t *pollSet);

void runClient(int socket, addrInfo_t *serverInfo, runtimeArgs_t *usrArgs, FILE *fp);

int diff(char *to_filename, char *from_filename) {

    /* TODO: REMOVE */

    FILE *to = fopen(to_filename, "r");
    FILE *from = fopen(from_filename, "r");
    int t = fgetc(to), f = fgetc(from);

    while (t != EOF && f != EOF) {

        if (fgetc(to) != fgetc(from)) return 1;

        t = fgetc(to);
        f = fgetc(from);
    }

    if (t != EOF || f != EOF) return 1;

    return 0;

}

int main(int argc, char *argv[]) {

    FILE *fp;
    runtimeArgs_t usrArgs = {0};
    int socket;
    addrInfo_t *serverInfo = initAddrInfo();

    /* Verify user arguments */
    checkArgs(argc, argv, &fp, &usrArgs);

    /* Set up the UDP connection */
    socket = setupUdpClientToServer(serverInfo, usrArgs.host_name, usrArgs.host_port);

    /* Initialize the error rate library */
    sendErr_init(usrArgs.error_rate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);
    printf("\n");

    /* Start the transfer process */
    runClient(socket, serverInfo, &usrArgs, fp);

    return 0;
}

void checkArgs(int argc, char *argv[], FILE **fp, runtimeArgs_t *usrArgs) {

    /* check command line arguments  */
    if (argc != 8) {
        fprintf(
                stderr,
                "\nusage: ./rcopy "
                "from-filename "
                "to-filename "
                "window-size "
                "buffer-size "
                "error-percent "
                "remote-machine "
                "remote-port \n"
        );
        exit(EXIT_FAILURE);
    }

    usrArgs->from_filename = strdup(argv[ARGV_FROM_FILENAME]);

    /* Check if from-filename is shorter than 100 characters */
    if (strlen(usrArgs->from_filename) > 99) {
        fprintf(stderr, "\nError: from-filename must be no longer than 100 characters. \n");
        exit(EXIT_FAILURE);
    }

    *fp = fopen(usrArgs->from_filename, "r");

    /* Check if the given file exists */
    if (*fp == NULL) {
        fprintf(stderr, "\nError: file %s not found. \n", usrArgs->from_filename);
        exit(EXIT_FAILURE);
    }

    usrArgs->to_filename = strdup(argv[ARGV_TO_FILENAME]);

    /* Check if to-filename is shorter than 100 characters */
    if (strlen(usrArgs->to_filename) > 99) {
        fprintf(stderr, "\nError: to-filename must be no longer than 100 characters. \n");
        exit(EXIT_FAILURE);
    }

    usrArgs->error_rate = strtof(argv[ARGV_ERR_RATE], NULL);

    /* Check that error rate is in a valid range */
    if (usrArgs->error_rate >= 1.0 || usrArgs->error_rate < 0) {
        fprintf(stderr, "\nError: error-rate must be greater than or equal to 0, and less than 1. \n");
        exit(EXIT_FAILURE);
    }

    usrArgs->buffer_size = strtol(argv[ARGV_BUFFER_SIZE], NULL, 10);

    /* Check buffer size is within range */
    if (usrArgs->buffer_size == 0 || usrArgs->buffer_size > 1400) {
        fprintf(stderr, "\nError: buffer-size must be between 1 and 1400 (inclusive). \n");
        exit(EXIT_FAILURE);
    }

    usrArgs->window_size = strtol(argv[ARGV_WINDOW_SIZE], NULL, 10);

    /* Check window size is within its range */
    if (usrArgs->window_size > 0x40000000) {
        fprintf(stderr, "\nError: window-size must be less than 2^30. \n");
        exit(EXIT_FAILURE);
    }

    usrArgs->host_name = strdup(argv[ARGV_HOST_NAME]);
    usrArgs->host_port = (uint16_t) strtol(argv[ARGV_HOST_PORT], NULL, 10);

    /* Make sure a port was given */
    if (usrArgs->host_port < 1) {
        fprintf(stderr, "\nError: host-port must be greater than 0. \n");
        exit(EXIT_FAILURE);
    }

}

/*!
 * Opens a socket and fills in the serverAddress struct using the hostName and serverPort. \n\n
 * Written by Hugh Smith – April 2017
 * @param serverInfo A non-null sockaddr_in6 struct that has been initialized to 0
 * @param hostName
 * @param hostPort
 * @return A UDP socket connected to a UDP server.
 */
int setupUdpClientToServer(addrInfo_t *serverInfo, char *hostName, int hostPort) {

    int clientSocket;
    char ipString[INET6_ADDRSTRLEN];
    uint8_t *ipAddress = NULL;

    /* Open a new socket */
    clientSocket = socket(AF_INET6, SOCK_DGRAM, 0);
    if (clientSocket == -1) {
        fprintf(stderr, "Couldn't open socket!\n");
        perror("socket() call error");
        exit(EXIT_FAILURE);
    }

    /* Add the port to the address info struct */
    *((int *) serverInfo->addrInfo->sa_data) = htons(hostPort);

    /* Get the IP address */
    ipAddress = gethostbyname6(hostName, (struct sockaddr_in6 *) serverInfo->addrInfo);

    if (ipAddress == NULL) {
        printf("IP address null?\n");
        exit(EXIT_FAILURE);
    }

    /* TODO: Remove */
    inet_ntop(AF_INET6, ipAddress, ipString, sizeof(ipString));
    printf("Server info:     \n"
           "\tIP: %s        \n"
           "\tPort: %d      \n"
           "\n",
           ipString, hostPort
    );

    return clientSocket;
}

int setupTransfer(int socket, addrInfo_t *serverInfo, runtimeArgs_t *usrArgs, pollSet_t *pollSet) {

    int pollSocket;
    uint16_t count = 1, filenameLen = strlen(usrArgs->to_filename) + 1;
    uint16_t payloadLen = sizeof(uint16_t) + sizeof(uint32_t) + filenameLen;
    uint8_t *hsPayload = scalloc(1, payloadLen);
    size_t pduLen;

    packet_t *hsPkt = initPacket(payloadLen);

    /* First, connect to the child socket */

    /* Add the buffer length to the payload */
    memcpy(&hsPayload[HS_IDX_BUFF_LEN], &(usrArgs->buffer_size), sizeof(uint16_t));

    /* Add the window size to the payload */
    memcpy(&hsPayload[HS_IDX_WIND_LEN], &(usrArgs->window_size), sizeof(uint32_t));

    /* Add the to-filename to the payload */
    memcpy(&hsPayload[HS_IDX_FILENAME], (usrArgs->to_filename), filenameLen);

    /* Fill the PDU */
    pduLen = buildPacket(hsPkt, payloadLen, 0, INFO_PKT, hsPayload, payloadLen);

    /* Wait for the server to respond */
    while (1) {

        /* If we get no response after 10 tries, assume the connection has been terminated */
        if (count > 10) {
            printf("Server has disconnected!\n");
            return 1;
        } else count++;

        /* Send the PDU */
        safeSendTo(socket, (void *) hsPkt, pduLen, serverInfo);

        /* Check poll */
        pollSocket = pollCall(pollSet, POLL_1_SEC);

        /* Check for time out */
        if (pollSocket >= 0) {

            /* Receive packet */
            pduLen = safeRecvFrom(pollSocket, (void *) hsPkt, MAX_PDU_LEN, serverInfo);

            /* Verify checksum */
            if (in_cksum((unsigned short *) hsPkt, (int) pduLen) == 0) {

                /* Check packet flag */
                if (hsPkt->flag == INFO_ACK_PKT) return 0;
                else if (hsPkt->flag == INFO_ERR_PKT) {

                    printf("The server was unable to create the new file, please try again. \n");
                    return 1;
                }
            }
        }
    }
}

void teardown(int socket, runtimeArgs_t *usrArgs, pollSet_t *pollSet, FILE *fp) {

    close(socket);

    freePollSet(pollSet);

    free(usrArgs->to_filename);
    free(usrArgs->from_filename);
    free(usrArgs->host_name);

    fclose(fp);
}

void runClient(int socket, addrInfo_t *serverInfo, runtimeArgs_t *usrArgs, FILE *fp) {

    uint8_t count = 0, eof = 0, buffLen = usrArgs->buffer_size, *dataBuff = malloc(buffLen + 1);
    uint32_t currentSeq = 0, recvSeq;
    size_t pktLen, readLen;
    ssize_t ret;

    packet_t *packet = NULL;
    circularWindow_t *packetWindow = NULL;

    pollSet_t *pollSet = initPollSet();

    /* Add the socket to the poll set */
    addToPollSet(pollSet, socket);

    /* Send the server the necessary transfer details */
    if (setupTransfer(socket, serverInfo, usrArgs, pollSet) != 0) {
        teardown(socket, usrArgs, pollSet, fp);
        return;
    }

    /* Set up the packet and window */
    packet = initPacket(usrArgs->buffer_size);
    packetWindow = createWindow(usrArgs->window_size, usrArgs->buffer_size);

    /* Transfer data */
    while (1) {

        /* Monitor connection */
        if (count > 9) {
            printf("Server has disconnected, shutting down...\n");
            teardown(socket, usrArgs, pollSet, fp);
            return;
        }

        /* Fill the window */
        while (eof == 0 && getWindowSpace(packetWindow)) {

            /* Read from the input file */
            readLen = (ssize_t) fread(dataBuff, sizeof(uint8_t), buffLen, fp);

            /* Check for EOF */
            if (feof(fp)) {

                /* Make a packet */
                pktLen = buildPacket(packet, buffLen, currentSeq++, DATA_EOF_PKT, dataBuff, readLen);

                /* Add it to the window */
                addWindowPacket(packetWindow, packet, pktLen);

                /* Prevent any more packets being added */
                eof = 1;

                break;
            }

            /* Make a packet */
            pktLen = buildPacket(packet, buffLen, currentSeq++, DATA_PKT, dataBuff, readLen);

            /* Add it to the window */
            addWindowPacket(packetWindow, packet, pktLen);
        }

        /* Check if we can send more packets */
        while (checkWindowOpen(packetWindow)) {

            /* Get the packet at current */
            pktLen = getWindowPacket(packetWindow, packet, WINDOW_CURRENT);

            /* Send the packet */
            safeSendTo(socket, (void *) packet, pktLen, serverInfo);

            /* Don't send anything in the window after the EOF packet */
            if (packet->flag == DATA_EOF_PKT) break;
        }

        /* Receive data */
        while (1) {

            /* Check if the window is open or closed */
            if (checkWindowOpen(packetWindow)) {

                /* Window open: non-blocking */
                ret = pollCall(pollSet, POLL_NON_BLOCK);

            } else {

                /* Window closed: blocking */
                ret = pollCall(pollSet, POLL_1_SEC);

            }

            /* Check if poll timed out */
            if (ret < 0) {

                /* If poll times out, resend the lowest packet */
                pktLen = getWindowPacket(packetWindow, packet, WINDOW_LOWER);

                /* Send packet */
                safeSendTo(socket, (void *) packet, pktLen, serverInfo);

                /* Keep track of timeouts */
                count++;

                printf("\nThere have been %d time-out(s)\n\n", count);

                /* Try resending */
                break;
            }

            /* Receive the packet */
            pktLen = safeRecvFrom(socket, packet, buffLen + PDU_HEADER_LEN, serverInfo);

            /* Verify checksum and packet type */
            if (in_cksum((unsigned short *) packet, (int) pktLen)) {

                /* If a server's reply becomes corrupt, we don't know what it wanted,
                 * so get the lowest packet in the window to get back on track */
                pktLen = getWindowPacket(packetWindow, packet, WINDOW_LOWER);

                /* Send packet */
                safeSendTo(socket, (void *) packet, pktLen, serverInfo);

            } else if (packet->flag == DATA_REJ_PKT) {

                /* Get the rejected packet's sequence number */
                recvSeq = htonl(*((uint32_t *) packet->payload));

                /* Get the requested packet */
                pktLen = getWindowPacket(packetWindow, packet, (int) recvSeq);

                /* Send the packet */
                safeSendTo(socket, (void *) packet, pktLen, serverInfo);

            } else if (packet->flag == DATA_ACK_PKT || packet->flag == TERM_CONN_PKT) {

                /* EOF waits until all RRs are received */
                if (eof == 1 && packet->flag != TERM_CONN_PKT) continue;

                /* Get the RRs sequence number */
                recvSeq = htonl(*((uint32_t *) packet->payload));

                /* Move the window to the packet after the received packet sequence */
                moveWindow(packetWindow, recvSeq + 1);

                /* Once the window moves, we can send another packet */
                break;
            }
            /* Packets without the Ack or SRej flag are ignored */

            /* Reset count */
            if (count > 0) {
                count = 0;
                printf("Count has been reset\n");
            }
        }

        /* Once a termination request has been received and all packets have been RRed, we're done sending data */
        if (packet->flag == TERM_CONN_PKT) break;
    }

    /* Send the Ack until the server stops asking for it, or until 10 attempts have been made */
    count = 0;
    while (1) {
        /* Only try 10 times */
        if (count > 9) break;

        /* Make a termination ACK packet */
        pktLen = buildPacket(packet, buffLen, currentSeq, TERM_ACK_PKT, NULL, 0);

        /* Send the packet */
        safeSendTo(socket, packet, pktLen, serverInfo);

        /* Check for a response */
        if (pollCall(pollSet, POLL_1_SEC) != POLL_TIMEOUT) {

            /* Clear the response from the recv buffer */
            safeRecvFrom(socket, packet, PDU_HEADER_LEN, serverInfo);

            /* Increment number of attempts */
            count++;

        } else break;
    }

    printf("File transfer has successfully completed!\n");

    /* TODO: REMOVE */
    if (diff(usrArgs->to_filename, usrArgs->from_filename)) {
        printf(
                "\n"
                "DIFF ERROR!!!\n"
                "DIFF ERROR!!!\n"
                "DIFF ERROR!!!\n"
        );
    }

    /* Clean up! */
    teardown(socket, usrArgs, pollSet, fp);
}
