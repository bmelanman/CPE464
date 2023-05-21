
/* Simple UDP Client
 * Created by Bryce Melander in May 2023
 * using modified code from Dr. Hugh Smith
 * */

#include "rcopy.h"
#include "libPoll.h"

void checkArgs(int argc, char *argv[], FILE **fp, runtimeArgs_t *usrArgs);

int setupUdpClientToServer(struct sockaddr_in6 *serverAddress, char *hostName, int hostPort);

void setupTransfer(int socket, struct sockaddr *srcAddr, int addrLen, runtimeArgs_t *usrArgs, pollSet_t *pollSet);

void runClient(int socket, struct sockaddr_in6 *server);

int main(int argc, char *argv[]) {

    struct sockaddr_in6 serverInfo = {0};
    int socket;
    FILE *fp;
    runtimeArgs_t usrArgs = {0};
    pollSet_t *pollSet = NULL;

    /* Verify user arguments */
    checkArgs(argc, argv, &fp, &usrArgs);

    /* TODO: Remove user prompt */
    printf(
            "\n"
            "Arguments:                 \n"
            "\tfromFilename: %s         \n"
            "\ttoFilename:   %s         \n"
            "\twindowSize:   %u         \n"
            "\tbufferSize:   %u         \n"
            "\terrorRate:    %3.2f%%    \n"
            "\thostName:     %s         \n"
            "\thostPort:     %d         \n"
            "\n"
            "Press ENTER key to Continue... \n\n",
            usrArgs.from_filename, usrArgs.to_filename,
            usrArgs.window_size, usrArgs.buffer_size, (usrArgs.error_rate * 100),
            usrArgs.host_name, usrArgs.host_port
    );
    getchar();

    /* Set up the UDP connection */
    socket = setupUdpClientToServer(&serverInfo, usrArgs.host_name, usrArgs.host_port);

    /* Set up the pollSet */
    pollSet = newPollSet();
    addToPollSet(pollSet, socket);

    /* Send the server the necessary transfer details */
    setupTransfer(socket,
                  (struct sockaddr *) &serverInfo, serverInfo.sin6_len, &usrArgs, pollSet);

    /* Initialize the error rate library */
    sendErr_init(usrArgs.error_rate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);

    addToPollSet(pollSet, STDIN_FILENO);

    /* Start the transfer process */
    runClient(socket, &serverInfo);

    /* Clean up */
    close(socket);
    fclose(fp);

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
    if (strlen(usrArgs->from_filename) > 100) {
        fprintf(stderr, "\nError: from-filename must be no longer than 100 characters. \n");
        exit(EXIT_FAILURE);
    }

    *fp = fopen(usrArgs->from_filename, "w");

    /* Check if the given file exists */
    if (*fp == NULL) {
        fprintf(stderr, "\nError: file %s not found. \n", usrArgs->from_filename);
        exit(EXIT_FAILURE);
    }

    usrArgs->to_filename = strdup(argv[ARGV_TO_FILENAME]);

    /* Check if to-filename is shorter than 100 characters */
    if (strlen(usrArgs->to_filename) > 100) {
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
    printf("Server info - IP: %s Port: %d \n", ipString, hostPort);

    return clientSocket;
}

void teardown(int socket, runtimeArgs_t *usrArgs, pollSet_t *pollSet) {

    close(socket);

    free(usrArgs->to_filename);
    free(usrArgs->from_filename);
    free(usrArgs->host_name);
    free(usrArgs);

    freePollSet(pollSet);

    exit(EXIT_SUCCESS);
}

void setupTransfer(int socket, struct sockaddr *srcAddr, int addrLen, runtimeArgs_t *usrArgs, pollSet_t *pollSet) {

    int pollSocket = -1;
    udpPacket_t handshakePacket = {0};
    uint16_t count = 0, packetLen, payloadLen = 0, filenameLen = strlen(usrArgs->to_filename);
    uint8_t handshakePayload[100] = {0};

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
    packetLen = createPDU(&handshakePacket, 0, INFO_PKT, handshakePayload, payloadLen);

    /* Wait for the server to respond */
    while (pollSocket == -1) {

        /* If we get no response after 10 tries, assume the connection has been terminated */
        if (count > 10) {
            teardown(socket, usrArgs, pollSet);
        } else {
            /* Send the PDU */
            safeSendTo(socket, (void *) &handshakePacket, packetLen, srcAddr, addrLen);

            /* Keep track of the number of attempts */
            count++;
        }

        /* Check poll */
        pollSocket = pollCall(pollSet, POLL_1_SEC);

        /* Receive packet */
        if (pollSocket > 0) {

            safeRecvFrom(pollSocket, (void *) &handshakePacket, MAX_PDU_LEN, 0, srcAddr, &addrLen);

            if (handshakePacket.flag == INFO_ACK_PKT) return;
        }
    }

}

int readFromStdin(char *buffer, int buffLen) {
    char aChar = 0;
    int inputLen = 0;

    printf("Enter data: ");

    /* Make sure we don't overflow the input buffer */
    while (inputLen < (buffLen - 1) && aChar != '\n') {

        aChar = (char) getchar();

        if (aChar != '\n') {
            buffer[inputLen] = aChar;
            inputLen++;
        }
    }

    /* Add null termination to the string */
    buffer[inputLen++] = '\0';

    return inputLen;
}

void runClient(int socket, struct sockaddr_in6 *server) {

    int serverAddrLen = sizeof(struct sockaddr_in6);
    int dataLen, pduLen, cnt = 0;
    uint8_t dataBuff[MAX_PAYLOAD_LEN] = {0}, pduBuff[MAX_PDU_LEN];
    udpPacket_t dataPDU = {0};

    while (dataBuff[0] != '.') {
        dataLen = readFromStdin((char *) dataBuff, MAX_PAYLOAD_LEN);

        pduLen = createPDU(&dataPDU, cnt++, 9, dataBuff, dataLen);

        safeSendTo(socket, pduBuff, pduLen, (struct sockaddr *) server, serverAddrLen);

        safeRecvFrom(socket, dataBuff, MAX_BUFF_LEN, 0, (struct sockaddr *) server, &serverAddrLen);


    }
}
