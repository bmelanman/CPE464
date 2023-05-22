
/* Simple UDP Client
 * Created by Bryce Melander in May 2023
 * using modified code from Dr. Hugh Smith
 * */

#include "rcopy.h"

void checkArgs(int argc, char *argv[], FILE **fp, runtimeArgs_t *usrArgs);

int setupUdpClientToServer(struct sockaddr_in6 *serverAddress, char *hostName, int hostPort);

int setupTransfer(int socket, struct sockaddr_in6 *srcAddr, int addrLen, runtimeArgs_t *usrArgs, pollSet_t *pollSet);

void runClient(int socket, struct sockaddr_in6 *serverInfo, runtimeArgs_t *usrArgs, FILE *fp);

int main(int argc, char *argv[]) {

    struct sockaddr_in6 serverInfo = {0};
    int socket;
    FILE *fp;
    runtimeArgs_t usrArgs = {0};

    /* Verify user arguments */
    checkArgs(argc, argv, &fp, &usrArgs);

//    /* TODO: Remove user prompt */
//    printf(
//            "\n"
//            "Arguments:                 \n"
//            "\tfromFilename: %s         \n"
//            "\ttoFilename:   %s         \n"
//            "\twindowSize:   %u         \n"
//            "\tbufferSize:   %u         \n"
//            "\terrorRate:    %3.2f%%    \n"
//            "\thostName:     %s         \n"
//            "\thostPort:     %d         \n"
//            "\n"
//            "Press ENTER key to Continue... \n",
//            usrArgs.from_filename, usrArgs.to_filename,
//            usrArgs.window_size, usrArgs.buffer_size, (usrArgs.error_rate * 100),
//            usrArgs.host_name, usrArgs.host_port
//    );
//    getchar();

    /* Set up the UDP connection */
    socket = setupUdpClientToServer(&serverInfo, usrArgs.host_name, usrArgs.host_port);

    /* Initialize the error rate library */
    sendErr_init(usrArgs.error_rate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);
    printf("\n");

    /* Start the transfer process */
    runClient(socket, &serverInfo, &usrArgs, fp);

    return 0;
}

int fpeek(FILE *stream) {
    int c = fgetc(stream);
    ungetc(c, stream);
    return c;
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
    if (*fp == NULL || fpeek(*fp) == EOF) {
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

void teardown(int socket, runtimeArgs_t *usrArgs, pollSet_t *pollSet, FILE *fp) {

    printf("Server has disconnected, shutting down...\n");

    close(socket);

    free(usrArgs->to_filename);
    free(usrArgs->from_filename);
    free(usrArgs->host_name);

    freePollSet(pollSet);

    fclose(fp);
}

int setupTransfer(int socket, struct sockaddr_in6 *srcAddr, int addrLen, runtimeArgs_t *usrArgs, pollSet_t *pollSet) {

    int pollSocket = -1;
    udpPacket_t handshakePacket = {0};
    uint16_t count = 1, payloadLen = 0, filenameLen = strlen(usrArgs->to_filename) + 1;
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
    createPDU(&handshakePacket, 0, INFO_PKT, handshakePayload, payloadLen);

    /* Wait for the server to respond */
    while (1) {

        /* If we get no response after 10 tries, assume the connection has been terminated */
        if (count > 10) return 1;
        else count++;

        /* Send the PDU */
        safeSendTo(socket, (void *) &handshakePacket, handshakePacket.pduLen, (struct sockaddr *) srcAddr, addrLen);

        /* Check poll */
        pollSocket = pollCall(pollSet, POLL_1_SEC);
        if (pollSocket < 0) continue;

        /* Receive packet */
        safeRecvFrom(pollSocket, (void *) &handshakePacket, MAX_PDU_LEN, 0, (struct sockaddr *) srcAddr, addrLen);

        /* Check for ACK flag */
        if (handshakePacket.flag == INFO_ACK_PKT) break;
    }

    return 0;
}

void runClient(int socket, struct sockaddr_in6 *serverInfo, runtimeArgs_t *usrArgs, FILE *fp) {

    int addrLen = sizeof(struct sockaddr_in6), pollSock, inc, payloadLen;
    uint32_t currentSeq = 0, i;
    uint8_t ret, count = 0, packetsSent = 0,
            buffLen = usrArgs->buffer_size, *dataBuff = malloc(buffLen);

    pollSet_t *pollSet = newPollSet();
    circularWindow_t *windowBuff = NULL;
    udpPacket_t dataPDU = {0};

    /* Set up the pollSet */
    addToPollSet(pollSet, socket);

    /* Send the server the necessary transfer details */
    ret = setupTransfer(socket, serverInfo, addrLen, usrArgs, pollSet);

    if (ret != 0) {
        teardown(socket, usrArgs, pollSet, fp);
        return;
    }

    windowBuff = createWindow(usrArgs->window_size, usrArgs->buffer_size);

    /* Transfer data */
    while (1) {

        if (count > 9) {
            teardown(socket, usrArgs, pollSet, fp);
            return;
        }

        /* Fill any space in the window */
        while (getWindowSpace(windowBuff)) {

            /* Read from the input file */
            fread(dataBuff, sizeof(uint8_t), buffLen, fp);

            /* Check if the file is empty */
            if (feof(fp)) break;

            /* Make a packet */
            createPDU(&dataPDU, currentSeq++, DATA_PKT, dataBuff, buffLen);

            /* Add it to the window */
            addPacket(windowBuff->circQueue, &dataPDU);
        }

        /* Check if the file is empty */
        if (feof(fp)) break;

        /* Check if we can send more packets */
        while (checkSendSpace(windowBuff)) {

            dataPDU = *getCurrentPacket(windowBuff);

            safeSendTo(socket, (void *) &dataPDU, dataPDU.pduLen, (struct sockaddr *) serverInfo, addrLen);

            incrementCurrent(windowBuff);
            packetsSent++;
        }

        pollSock = pollCall(pollSet, POLL_1_SEC);

        if (pollSock < 0) {
            count++;
            decrementCurrent(windowBuff, packetsSent);
            continue;
        }

        /* Wait for ACKs */
        while (packetsSent) {

            /* Get the header, then the rest of the data */
            safeRecvFrom(pollSock, &dataPDU, MAX_PDU_LEN, 0, (struct sockaddr *) serverInfo, addrLen);

//            payloadLen = dataPDU.pduLen - PDU_HEADER_LEN;
//
//            if (payloadLen) {
//                safeRecvFrom(pollSock, &dataPDU.payload, payloadLen, 0, (struct sockaddr *) serverInfo, addrLen);
//            }

            if (in_cksum((unsigned short *) &dataPDU, dataPDU.pduLen) != 0) {
                continue;
            }

            inc = (int) ntohl(dataPDU.seq_NO) - getIncrement(windowBuff) + 1;

            moveWindow(windowBuff, inc);
            packetsSent -= inc;
        }

    }

    /* Read the last of the data */
    fread(dataBuff, sizeof(uint8_t), buffLen, fp);

    /* Make an EOF packet */
    createPDU(&dataPDU, currentSeq, DATA_EOF_PKT, dataBuff, buffLen);

    /* Send the packet */
    safeSendTo(pollSock, &dataPDU, dataPDU.pduLen, (struct sockaddr *) serverInfo, addrLen);

    printf("File transfer has successfully completed!\n");

    /* Clean up */
    teardown(socket, usrArgs, pollSet, fp);
}
