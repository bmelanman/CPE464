
/* Simple UDP Client
 * Created by Bryce Melander in May 2023
 * using modified code from Dr. Hugh Smith
 * */

#include "rcopy.h"

void checkArgs(int argc, char *argv[], FILE **fp, runtimeArgs_t *usrArgs);

int setupUdpClientToServer(addrInfo_t *serverInfo, char *hostName, int hostPort);

int setupTransfer(int socket, addrInfo_t *dstInfo, runtimeArgs_t *usrArgs, pollSet_t *pollSet);

void runClient(int socket, addrInfo_t *serverInfo, runtimeArgs_t *usrArgs, FILE *fp);

/* TODO: REMOVE */
int diff(char *to_filename, char *from_filename) {

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

void teardown(int socket, runtimeArgs_t *usrArgs, FILE *fp) {

    close(socket);

    free(usrArgs->to_filename);
    free(usrArgs->from_filename);
    free(usrArgs->host_name);

    fclose(fp);
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

    if (diff(usrArgs.to_filename, usrArgs.from_filename)) {
        printf("\nDIFF ERROR!!!\n");
        printf("DIFF ERROR!!!\n");
        printf("DIFF ERROR!!!\n");
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
 * @param serverInfo A non-null sockaddr_in6 struct that has been initialized to 0
 * @param hostName
 * @param hostPort
 * @return A UDP socket connected to a UDP server.
 */
int setupUdpClientToServer(addrInfo_t *serverInfo, char *hostName, int hostPort) {

    int clientSocket;
    char ipString[INET6_ADDRSTRLEN];
    uint8_t *ipAddress = NULL;

//    struct sockaddr_in6 tempAddr = {0};

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

int setupTransfer(int socket, addrInfo_t *dstInfo, runtimeArgs_t *usrArgs, pollSet_t *pollSet) {

    int pollSocket;
    uint16_t count = 1, pduLen, filenameLen = strlen(usrArgs->to_filename) + 1;
    uint16_t payloadLen = sizeof(uint16_t) + sizeof(uint32_t) + filenameLen;
    uint8_t *handshakePayload = scalloc(1, payloadLen);

    udpPacket_t *handshakePacket = initPacket(payloadLen);

    /* Add the buffer length to the payload */
    memcpy(&handshakePayload[HS_IDX_BUFF_LEN], &(usrArgs->buffer_size), sizeof(uint16_t));

    /* Add the window size to the payload */
    memcpy(&handshakePayload[HS_IDX_WIND_LEN], &(usrArgs->window_size), sizeof(uint32_t));

    /* Add the to-filename to the payload */
    memcpy(&handshakePayload[HS_IDX_FILENAME], (usrArgs->to_filename), filenameLen);

    /* Fill the PDU */
    pduLen = createPDU(handshakePacket, payloadLen, 0, INFO_PKT, handshakePayload, payloadLen);

    /* Wait for the server to respond */
    while (1) {

        /* If we get no response after 10 tries, assume the connection has been terminated */
        if (count > 10) return 1;
        else count++;

        /* Send the PDU */
        safeSendTo(socket, (void *) handshakePacket, pduLen, dstInfo);

        /* Check poll */
        pollSocket = pollCall(pollSet, POLL_1_SEC);
        if (pollSocket < 0) continue;

        /* Receive packet */
        pduLen = safeRecvFrom(pollSocket, (void *) handshakePacket, MAX_PDU_LEN, dstInfo);

        /* Check for ACK flag */
        if (pduLen > 0 && handshakePacket->flag == INFO_ACK_PKT) break;
    }

    return 0;
}

void runClient(int socket, addrInfo_t *serverInfo, runtimeArgs_t *usrArgs, FILE *fp) {

    uint8_t count = 0, eof = 0, buffLen = usrArgs->buffer_size, *dataBuff = malloc(buffLen + 1);
    uint16_t readLen, pduLen;
    uint32_t currentSeq = 0, recvSeq;

    pollSet_t *pollSet = newPollSet();
    circularWindow_t *packetWindow = NULL;
    udpPacket_t *dataPDU = initPacket(usrArgs->buffer_size);

    /* Set up the pollSet */
    addToPollSet(pollSet, socket);

    /* Send the server the necessary transfer details */
    if (setupTransfer(socket, serverInfo, usrArgs, pollSet) != 0) {
        printf("Server has disconnected, shutting down...\n");
        freePollSet(pollSet);
        return;
    }

    /* Set up the packet window */
    packetWindow = createWindow(usrArgs->window_size, usrArgs->buffer_size);

    /* Transfer data */
    while (1) {

        if (count > 9) {
            printf("Server has disconnected, shutting down...\n");
            freePollSet(pollSet);
            return;
        }

        /* Fill the window */
        while (eof == 0 && getWindowSpace(packetWindow)) {

            /* Read from the input file */
            readLen = fread(dataBuff, sizeof(uint8_t), buffLen, fp);

            /* Check for EOF */
            if (feof(fp)) {

                /* Make a packet */
                pduLen = createPDU(dataPDU, buffLen, currentSeq++, DATA_EOF_PKT, dataBuff, readLen);

                /* Add it to the window */
                addWindowPacket(packetWindow, dataPDU, pduLen);

                /* Prevent any more packets being added */
                eof = 1;

                break;
            }

            /* Make a packet */
            pduLen = createPDU(dataPDU, buffLen, currentSeq++, DATA_PKT, dataBuff, readLen);

            /* Add it to the window */
            addWindowPacket(packetWindow, dataPDU, pduLen);
        }

        /* Check if we can send more packets */
        while (checkSendSpace(packetWindow)) {

            /* Get the packet at current */
            pduLen = getCurrentPacket(packetWindow, dataPDU);

            /* Send the packet */
            safeSendTo(socket, (void *) dataPDU, pduLen, serverInfo);

            /* Increment the current packet index */
            incrementCurrent(packetWindow);
        }

        /* Receive data */
        while (1) {

            /* Check if poll timed out */
            if (pollCall(pollSet, POLL_1_SEC) < 0) {

                /* If poll times out, current is reset to the lowest packet */
                resetCurrent(packetWindow);

                /* Keep track of timeouts */
                count++;

                printf("\nThere have been %d time-out(s)\n\n", count);

                /* Try resending */
                break;
            }

            /* Receive the packet */
            pduLen = safeRecvFrom(socket, dataPDU, buffLen + PDU_HEADER_LEN, serverInfo);

            /* Verify checksum and packet type */
            if (in_cksum((unsigned short *) dataPDU, pduLen)) {

                /* If a server's reply becomes corrupt, we don't know what it wanted,
                 * so get the lowest packet in the window to get back on track */
                pduLen = getLowestPacket(packetWindow, dataPDU);

                /* Send packet */
                safeSendTo(socket, (void *) dataPDU, pduLen, serverInfo);

            } else if (dataPDU->flag == DATA_REJ_PKT) {

                /* Get the rejected packet's sequence number */
                recvSeq = htonl(*((uint32_t *) dataPDU->payload));

                /* Get the requested packet */
                pduLen = getSeqPacket(packetWindow, recvSeq, dataPDU);

                /* Send the packet */
                safeSendTo(socket, (void *) dataPDU, pduLen, serverInfo);

            } else {

                /* EOF waits until all RRs are received */
                if (eof == 1 && dataPDU->flag != TERM_CONN_PKT) continue;

                /* Get the RRs sequence number */
                recvSeq = htonl(*((uint32_t *) dataPDU->payload));

                /* Move the window to the packet after the received packet sequence */
                moveWindow(packetWindow, recvSeq + 1);

                /* Once the window moves, we can send another packet */
                break;
            }

            if (count > 0) {
                /* Reset count */
                count = 0;
                printf("Count has been reset\n");
            }
        }

        if (dataPDU->flag == TERM_CONN_PKT) break;
    }

    /* Send a termination ACK */
    pduLen = createPDU(dataPDU, buffLen, currentSeq, TERM_ACK_PKT, NULL, 0);

    safeSendTo(socket, dataPDU, pduLen, serverInfo);

    printf("File transfer has successfully completed!\n");
}
