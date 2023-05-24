
/* Simple UDP Client
 * Created by Bryce Melander in May 2023
 * using modified code from Dr. Hugh Smith
 * */

#include "rcopy.h"

void checkArgs(int argc, char *argv[], FILE **fp, runtimeArgs_t *usrArgs);

int setupUdpClientToServer(struct sockaddr_in6 *serverAddress, char *hostName, int hostPort);

int setupTransfer(int socket, struct sockaddr_in6 *srcAddr, int addrLen, runtimeArgs_t *usrArgs, pollSet_t *pollSet);

void runClient(int socket, struct sockaddr_in6 *serverInfo, runtimeArgs_t *usrArgs, FILE *fp);

/* TODO: REMOVE */
int diff(char *to_filename, char *from_filename) {

    FILE *to = fopen(to_filename, "r");
    FILE *from = fopen(from_filename, "r");
    int t = fgetc(to), f = fgetc(from);

    while (t != EOF && f != EOF) {

        t = fgetc(to);
        f = fgetc(from);

        if (fgetc(to) != fgetc(from)) return 1;
    }

    if (t != EOF || f != EOF) return 1;

    return 0;

}

void teardown(int socket, runtimeArgs_t *usrArgs, FILE *fp) {

    close(socket);

    free(usrArgs->to_filename);
    free(usrArgs->from_filename);
    free(usrArgs->host_name);

    fclose(fp);
}

int main(int argc, char *argv[]) {

    struct sockaddr_in6 serverInfo = {0};
    int socket;
    FILE *fp;
    runtimeArgs_t usrArgs = {0};

    /* Verify user arguments */
    checkArgs(argc, argv, &fp, &usrArgs);

    /* Set up the UDP connection */
    socket = setupUdpClientToServer(&serverInfo, usrArgs.host_name, usrArgs.host_port);

    /* Initialize the error rate library */
    sendErr_init(usrArgs.error_rate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);
    printf("\n");

    /* Start the transfer process */
    runClient(socket, &serverInfo, &usrArgs, fp);

    if (diff(usrArgs.to_filename, usrArgs.from_filename)) {
        printf("\nDIFF ERROR!!!\n");
        printf("DIFF ERROR!!!\n");
        printf("DIFF ERROR!!!\n");
        exit(EXIT_FAILURE);
    }

    /* Clean up */
    teardown(socket, &usrArgs, fp);

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
 * @param serverAddress A non-null sockaddr_in6 struct that has been initialized to 0
 * @param hostName
 * @param hostPort
 * @return A UDP socket connected to a UDP server.
 */
int setupUdpClientToServer(struct sockaddr_in6 *serverAddress, char *hostName, int hostPort) {

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

    serverAddress->sin6_port = ntohs(hostPort);
    serverAddress->sin6_family = AF_INET6;

    ipAddress = gethostbyname6(hostName, serverAddress);

    if (ipAddress == NULL) {
        exit(EXIT_FAILURE);
    }

    inet_ntop(AF_INET6, ipAddress, ipString, sizeof(ipString));
    printf("Server info:     \n"
           "\tIP: %s        \n"
           "\tPort: %d      \n"
           "\n",
           ipString, hostPort
    );

    return clientSocket;
}

int setupTransfer(int socket, struct sockaddr_in6 *srcAddr, int addrLen, runtimeArgs_t *usrArgs, pollSet_t *pollSet) {

    int pollSocket;
    udpPacket_t handshakePacket = {0};
    uint16_t pduLen, count = 1, payloadLen = 0, filenameLen = strlen(usrArgs->to_filename) + 1;
    uint8_t handshakePayload[110] = {0};

    /* Add the buffer length to the payload */
    memcpy(&handshakePayload[payloadLen], &(usrArgs->buffer_size), sizeof(uint16_t));
    payloadLen += sizeof(uint16_t);

    /* Add the window size to the payload */
    memcpy(&handshakePayload[payloadLen], &(usrArgs->window_size), sizeof(uint32_t));
    payloadLen += sizeof(uint32_t);

    /* Add the to-filename to the payload */
    memcpy(&handshakePayload[payloadLen], (usrArgs->to_filename), filenameLen);
    payloadLen += filenameLen;

    /* Fill the PDU */
    pduLen = createPDU(&handshakePacket, 0, INFO_PKT, handshakePayload, payloadLen);

    /* Wait for the server to respond */
    while (1) {

        /* If we get no response after 10 tries, assume the connection has been terminated */
        if (count > 10) return 1;
        else count++;

        /* Send the PDU */
        safeSendTo(socket, (void *) &handshakePacket, pduLen,
                   (struct sockaddr *) srcAddr, addrLen);

        /* Check poll */
        pollSocket = pollCall(pollSet, POLL_1_SEC);
        if (pollSocket < 0) continue;

        /* Receive packet */
        pduLen = safeRecvFrom(pollSocket, (void *) &handshakePacket, MAX_PDU_LEN, 0,
                     (struct sockaddr *) srcAddr, addrLen);

        /* Check for ACK flag */
        if (pduLen > 0 && handshakePacket.flag == INFO_ACK_PKT) break;
    }

    return 0;
}

void runClient(int socket, struct sockaddr_in6 *serverInfo, runtimeArgs_t *usrArgs, FILE *fp) {

    int addrLen = sizeof(struct sockaddr_in6), pollSock, rejPkt = -1;
    uint8_t count = 0, buffLen = usrArgs->buffer_size + PDU_HEADER_LEN, *dataBuff = malloc(
            buffLen + 1);
    uint16_t readLen, pduLen;
    uint32_t currentSeq = 0, recvPkt;

    pollSet_t *pollSet = newPollSet();
    circularWindow_t *packetWindow = NULL;
    udpPacket_t dataPDU = {0};

    /* Set up the pollSet */
    addToPollSet(pollSet, socket);

    /* Send the server the necessary transfer details */
    if (setupTransfer(socket, serverInfo, addrLen, usrArgs, pollSet) != 0) {
        printf("Server has disconnected, shutting down...\n");
        freePollSet(pollSet);
        return;
    }

    packetWindow = createWindow(usrArgs->window_size);

    /* Transfer data */
    while (1) {

        if (count > 9) {
            printf("Server has disconnected, shutting down...\n");
            freePollSet(pollSet);
            return;
        }

        /* Fill any space in the window */
        while (getWindowSpace(packetWindow)) {

            /* Read from the input file */
            readLen = fread(dataBuff, sizeof(uint8_t), buffLen - PDU_HEADER_LEN, fp);

            /* Check EOF */
            if (feof(fp)) {

                /* Make an EOF packet */
                pduLen = createPDU(&dataPDU, currentSeq++, DATA_EOF_PKT, dataBuff, readLen);

                /* Add it to the window */
                addWindowPacket(packetWindow, &dataPDU, pduLen);

                break;
            }

            /* Make a packet */
            pduLen = createPDU(&dataPDU, currentSeq++, DATA_PKT, dataBuff, readLen);

            /* Add it to the window */
            addWindowPacket(packetWindow, &dataPDU, pduLen);
        }

        /* Check if we can send more packets */
        while (checkSendSpace(packetWindow)) {

            pduLen = getCurrentPacket(packetWindow, &dataPDU);

            safeSendTo(socket, (void *) &dataPDU, pduLen, (struct sockaddr *) serverInfo, addrLen);

            incrementCurrent(packetWindow);
        }

        /* Recv data */
        while (1) {

            /* Wait for a packet */
            pollSock = pollCall(pollSet, POLL_1_SEC);

            /* Check if poll timed out */
            if (pollSock < 0) {

                /* If poll times out, the whole window must be reset to lowest */
                moveCurrentToSeq(packetWindow, 0);

                /* Keep track of timeouts */
                count++;
                break;
            }

            /* Get the header, then the rest of the data */
            pduLen = safeRecvFrom(pollSock, &dataPDU, buffLen, 0, (struct sockaddr *) serverInfo, addrLen);

            /* Verify checksum */
            if (in_cksum((unsigned short *) &dataPDU, pduLen)) {

                /* If a server's reply becomes corrupt, we don't know what it wanted,
                 * so we'll send the lowest packet in the window to get back on track */

                /* Reset the window to lower */
                moveCurrentToSeq(packetWindow, 0);
                break;
            }

            /* Check for SRej */
            if (dataPDU.flag == DATA_REJ_PKT) {

                rejPkt = htonl(*((uint32_t *) dataPDU.payload));

                /* Get the requested packet */
                dataPDU = *getSeqPacket(packetWindow, rejPkt);

                /* Send the packet */
                safeSendTo(pollSock, (void *) &dataPDU, MAX_PDU_LEN, (struct sockaddr *) serverInfo, addrLen);

            } else {

                recvPkt = htonl(*((uint32_t *) dataPDU.payload));

                /* If we received a SRej, move current to the RRs next packet */
                if (rejPkt != -1) {

                    moveCurrentToSeq(packetWindow, recvPkt + 1);

                    rejPkt = -1;
                }

                /* Move the window to the packet after the received packet sequence */
                moveWindow(packetWindow, recvPkt + 1);

                /* Once the window moves, we can send another packet */
                break;
            }
        }

        /* Check for EOF and that all packets have been RR'ed */
        if (feof(fp) && recvPkt == (currentSeq - 3)) {

            pollSock = pollCall(pollSet, 1000);

            /* Check for any extra packets */
            if (pollSock == POLL_TIMEOUT) break;
        }

    }

    /* Send a termination ACK */
    pduLen = createPDU(&dataPDU, currentSeq, TERM_ACK_PKT, NULL, 0);

    /* Send the packet */
    safeSendTo(pollSock, &dataPDU, pduLen, (struct sockaddr *) serverInfo, addrLen);

    printf("File transfer has successfully completed!\n");
}
