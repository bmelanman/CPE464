
#include "cclient.h"

#include <ctype.h>
#include <unistd.h>

/* Private prototypes */
void checkArgs(int argc, char *argv[]);

int clientInitTCP(char *handle, char *server_name, char *server_port);

void clientControl(int clientSocket, char *handle);

int main(int argc, char *argv[]) {

    int socket;

    /* Check input arguments */
    checkArgs(argc, argv);

    /* Set up the TCP Client socket with the handle, server name, and port  */
    socket = clientInitTCP(argv[1], argv[2], argv[3]);

    /* User prompt */
    printf("$: ");
    fflush(stdout);

    /* Run the client */
    clientControl(socket, argv[1]);

    return 0;
}

/* Functions */

void checkArgs(int argc, char *argv[]) {

    /* Check command line arguments  */
    if (argc != 4) {
        fprintf(stderr, "Usage: %s handle serverName serverPort \n", argv[0]);
        exit(EXIT_FAILURE);
    }

    /* Make sure the handle starts with a letter */
    if (!isalpha(argv[1][0])) {
        fprintf(stderr, "Invalid handle, handle starts with a number. \n");
        exit(EXIT_FAILURE);
    }

    if (strlen(argv[1]) > 100) {
        fprintf(stderr, "Invalid handle, handle longer than 100 characters. \n");
        exit(EXIT_FAILURE);
    }

    printf("\n");
}

int sendToServer(int socket, uint8_t *sendBuf, int sendLen, uint8_t flag) {

    int bytesSent;

    /* Send the data as a packet with a header */
    bytesSent = sendPDU(socket, sendBuf, sendLen, flag);

    /* Make sure the server is still connected */
    checkSocketDisconnected(bytesSent, NULL, NULL, socket);

    return bytesSent;
}

int recvFromServer(int socket, uint8_t *recvBuff, uint16_t buffLen) {

    int bytesRecv = recvPDU(socket, recvBuff, buffLen);

    /* Make sure the server is still connected */
    checkSocketDisconnected(bytesRecv, NULL, NULL, socket);

    return bytesRecv;
}

void sendHandshake(int clientSocket, char *clientHandle) {

    uint8_t handleLen = (uint8_t) strnlen(clientHandle, 100);
    uint8_t sendLen = handleLen + 1;
    uint8_t sendBuf[MAX_BUF];

    /* 1 Byte: Handle length */
    memcpy(sendBuf, &handleLen, 1);
    /* N Bytes: The client handle*/
    memcpy(sendBuf + 1, clientHandle, handleLen);

    /* Send the packet */
    sendToServer(clientSocket, sendBuf, sendLen, CONN_PKT);
}

void recvHandshake(int clientSocket) {

    uint8_t flag, recvBuff[MAX_BUF];

    /* Wait for the server to send an ACK */
    printf("Waiting for server connection response...\n");
    recvFromServer(clientSocket, recvBuff, MAX_BUF);

    /* Get the packet flag */
    flag = recvBuff[PDU_FLAG];

    switch (flag) {
        case 2:     /* Successful connection */
            printf("Acknowledgement received, server connection successful!\n");
            break;
        case 3:     /* Server declined due to invalid handle */
            fprintf(stderr, "Server declined connection due to an invalid handle, please try again.\n");
            close(clientSocket);
            exit(EXIT_FAILURE);
        default:    /* Safety error handling */
            fprintf(stderr, "Packet with unknown flag \'%d\' received, terminating...", flag);
            close(clientSocket);
            exit(EXIT_FAILURE);
    }

}

int clientInitTCP(char *handle, char *server_name, char *server_port) {

    int clientSocket, err;
    struct addrinfo *info, hints = {
            0,
            AF_INET6,
            SOCK_STREAM,
            IPPROTO_IP,
            0,
            NULL,
            NULL,
            NULL
    };

    /* Get hostname of server_net_init */
    err = getaddrinfo(server_name, server_port, &hints, &info);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo() err #%d:\n", err);
        fprintf(stderr, "%s\n", gai_strerror(err));
        exit(EXIT_FAILURE);
    }

    /* Open a new socket */
    clientSocket = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (clientSocket == -1) {
        fprintf(stderr, "Couldn't open socket!\n");
        exit(EXIT_FAILURE);
    }

    /* Connect the socket */
    err = connect(clientSocket, info->ai_addr, info->ai_addrlen);
    if (err != 0) {
        fprintf(stderr, "Could not connect to server '%s' at port %s!\n",
                server_name, server_port
        );
        exit(EXIT_FAILURE);
    }

    /* Send the server a handshake packet */
    sendHandshake(clientSocket, handle);

    /* Receive the server response packet */
    recvHandshake(clientSocket);

    /* Clean up */
    freeaddrinfo(info);

    printf(
            "Your Handle: %s\n"
            "Server Name: %s\n"
            "Server Port: %s\n"
            "\n",
            handle, server_name, server_port
    );

    return clientSocket;
}

void processMessage(uint8_t dataBuff[]) {

    char srcHandle[MAX_HDL], msg[MAX_MSG];
    int dataLen, offset = 2;

    /* Get the length of the src handle */
    dataLen = dataBuff[1];

    /* Get the src handle */
    memcpy(srcHandle, &dataBuff[offset], dataLen);
    srcHandle[dataLen] = '\0';

    /* Get the length of the dst handle */
    offset += dataLen + 1;
    dataLen = dataBuff[offset];

    /* Get the message */
    offset += dataLen + 1;

    /* Check for an empty message */
    if (dataBuff[offset] == '\0') {
        snprintf(msg, 16, "<empty message>");
    } else {
        snprintf(msg, MAX_MSG, "%s", &dataBuff[offset]);
    }

    printf("\n%s: %s\n$: ", srcHandle, msg);
    fflush(stdout);

}

void processBroadcast(uint8_t dataBuff[]) {

    /* Unpack the packet */
    int handleLen = dataBuff[1];
    char handle[MAX_HDL], msg[MAX_MSG];

    memcpy(handle, &dataBuff[2], handleLen);
    handle[handleLen] = '\0';

    if (dataBuff[handleLen + 2] == '\0') {
        snprintf(msg, 16, "<empty message>");
    } else {
        snprintf(msg, MAX_MSG, "%s", (char *) &dataBuff[handleLen + 2]);
    }

    /* Print the info */
    printf("\n%s: %s\n$: ", handle, msg);
    fflush(stdout);
}

void processMulticast(uint8_t dataBuff[]) {

    int i;
    uint8_t srcHandleLen, offset = 1;
    char srcHandle[MAX_HDL], msg[MAX_BUF];

    /* Get the source info */
    srcHandleLen = dataBuff[offset++];

    /* Grab the src handle */
    memcpy(srcHandle, &dataBuff[offset], srcHandleLen);
    srcHandle[srcHandleLen] = '\0';
    offset += srcHandleLen;

    /* Get the number of destinations */
    uint8_t numDests = dataBuff[offset++];

    /* Skip through all the dst handles */
    for (i = 0; i < numDests; ++i) {
        offset += dataBuff[offset] + 1;
    }

    /* Get the message */
    if (dataBuff[offset] == '\0') {
        snprintf(msg, 16 + MAX_HDL, "%s: <empty message>\n", srcHandle);
    } else {
        snprintf(msg, MAX_MSG + MAX_HDL, "\n%s: %s\n", srcHandle, (char *) &dataBuff[offset]);
    }

    printf("%s$: ", msg);
    fflush(stdout);
}

void recvClientList(int clientSocket, uint8_t dataBuff[], int buffLen) {

    int handleLen;
    uint32_t numClients = ntohl(*((uint32_t *) &dataBuff[1]));
    char handle[MAX_HDL];

    printf("\nNumber of clients: %u\n", numClients);
    fflush(stdout);

    while (1) {

        /* Get the next handle packet */
        recvFromServer(clientSocket, dataBuff, buffLen);

        /* Check for done flag */
        if (dataBuff[PDU_FLAG] == FIN_LIST_PKT) break;

        /* Get the handle length */
        handleLen = dataBuff[0];

        /* Get the handle */
        memcpy(handle, &dataBuff[1], handleLen);
        handle[handleLen] = '\0';

        /* Print out the handle */
        printf(" * %s\n", handle);
    }

    /* Print out the handle */
    printf("$: ");
    fflush(stdout);

}

void processError(uint8_t recvBuff[]) {

    printf(
            "\nClient with handle %s does not exist.\n$: ",
            strndup(
                    (char *) (recvBuff + 1),
                    recvBuff[PDU_SRC_LEN_IDX] + 1)
    );
    fflush(stdout);

}

int processPacket(int socket, char *handle) {

    uint8_t recvBuff[MAX_BUF] = {0};

    /* Receive a new packet */
    recvFromServer(socket, recvBuff, MAX_BUF);

    switch (recvBuff[PDU_FLAG]) {
        case 3: /* Duplicate header error */
            printf("\nHandle already in use: %s\n", handle);
            return 1;
        case 4: /* Broadcast packet received */
            processBroadcast(recvBuff);
            break;
        case 5: /* Message packet received */
            processMessage(recvBuff);
            break;
        case 6: /* Multicast packet received */
            processMulticast(recvBuff);
            break;
        case 7: /* Message error packet */
            processError(recvBuff);
            break;
        case 9: /* Exit ACK packet */
            printf("\nExit request approved, shutting down...\n");
            return 1;
        case 11:
            recvClientList(socket, recvBuff, MAX_BUF);
    }

    return 0;
}

void packMessage(int socket, char *clientHandle, uint8_t *usrInput) {

    uint8_t sendBuff[MAX_BUF] = {0};
    uint8_t dataBuffLen = 0, strLen, numDests = 1;
    char *dstHandle = NULL, usrMsg[MAX_MSG];

    /* Get the handle */
    dstHandle = strtok((char *) &(usrInput[3]), " ");

    strLen = (uint8_t) strnlen(dstHandle, MAX_HDL - 1) + 1;
    snprintf(usrMsg, MAX_MSG, "%s", (char *) &(usrInput[strLen + 3]));

    /* Lengths of each string sent */
    if (clientHandle == NULL) {
        fprintf(stderr, "Error, client handle is NULL!\n");
        exit(EXIT_FAILURE);
    } else if (dstHandle == NULL) {
        fprintf(stderr, "Invalid command format: A destination handle is required.\n");
        return;
    }

    /* 1 Byte: Client handle length */
    strLen = (uint8_t) strlen(clientHandle);
    memcpy(&sendBuff[dataBuffLen++], &strLen, 1);
    /* N Bytes: Client handle */
    memcpy(&sendBuff[dataBuffLen], clientHandle, strLen);
    dataBuffLen += strLen;

    /* 1 Byte: Number of destinations */
    memcpy(&sendBuff[dataBuffLen++], &numDests, 1);

    /* 1 Byte: Destination handle length */
    strLen = (uint8_t) strlen(dstHandle);
    memcpy(&sendBuff[dataBuffLen++], &strLen, 1);
    /* N Bytes: Destination handle */
    memcpy(&sendBuff[dataBuffLen], dstHandle, strLen);
    dataBuffLen += strLen;

    /* 1 Byte: Message length */
    strLen = (uint8_t) strlen(usrMsg);
    /* N Bytes: The message with null terminator */
    memcpy(&sendBuff[dataBuffLen], usrMsg, ++strLen);
    dataBuffLen += strLen;

    /* Send the packet! */
    sendToServer(socket, sendBuff, dataBuffLen, MESSAGE_PKT);

}

void packBroadcast(int socket, char *handle, char *msg) {

    uint8_t dataBuff[MAX_BUF];

    /* Add handle length and handle */
    int handleLen = (int) strnlen(handle, MAX_HDL), msgLen;
    memcpy(dataBuff, &handleLen, 1);
    memcpy(dataBuff + 1, handle, handleLen++);

    /* Add the message */
    msgLen = (int) strnlen(msg, MAX_MSG) + 1;
    memcpy(&dataBuff[handleLen], msg, msgLen);

    /* Sent the packet */
    sendPDU(socket, dataBuff, handleLen + msgLen + 1, BROADCAST_PKT);

}

void packMulticast(int socket, char *handle, uint8_t usrInput[]) {

    uint8_t i, sendBuff[MAX_BUF] = {0}, handleLen, buffLen = 1, numDestinations, msgOffset = 5;
    char *dstHandle = NULL, msg[MAX_MSG];

    /* Add source handle and length */
    handleLen = strnlen(handle, MAX_HDL);
    memcpy(sendBuff, &handleLen, 1);
    memcpy(&sendBuff[buffLen], handle, handleLen);
    buffLen += handleLen;

    /* Add the number of destinations */
    numDestinations = strtol(strtok((char *) &usrInput[3], " "), NULL, 10);
    memcpy(&sendBuff[buffLen++], &numDestinations, 1);

    for (i = 0; i < numDestinations; i++) {

        /* Get the next dst handle */
        dstHandle = strtok(NULL, " ");

        /* Check for valid input */
        if (dstHandle == NULL) {
            fprintf(stderr, "Invalid command format: Requested %d destinations but only %d given.\n$: ",
                    numDestinations, i);
            fflush(stdout);
            return;
        }
        handleLen = strnlen(dstHandle, MAX_HDL);
        memcpy(&sendBuff[buffLen++], &handleLen, 1);
        memcpy(&sendBuff[buffLen], dstHandle, handleLen);
        buffLen += handleLen;

        msgOffset += handleLen + 1;

    }

    /* Add the message */
    snprintf(msg, MAX_MSG, "%s", &usrInput[msgOffset]);
    handleLen = strnlen(msg, MAX_MSG);
    memcpy(&sendBuff[buffLen], msg, ++handleLen);
    buffLen += handleLen;

    /* Send the packet */
    sendPDU(socket, sendBuff, buffLen, MULTICAST_PKT);

}

void reqList(int socket) {

    /* Send a close connection request packet */
    sendToServer(socket, NULL, 0, REQ_LIST_PKT);

}

void reqClose(int socket) {

    /* Send a close connection request packet */
    sendToServer(socket, NULL, 0, REQ_EXIT_PKT);

}

int checkValidInput(uint8_t usrInput[]) {

    /* Check for proper formatting */
    uint8_t cmd = tolower(usrInput[USR_CMD]);

    /* There should always be some form of whitespace after each command */
    if (!isspace(usrInput[CMD_SPC])) {
        fprintf(stderr, "Invalid command format\n");
        fflush(stdout);
        return 1;
    }

    switch (cmd) {
        /* These commands are valid at this point */
        case 'l':
        case 'e':
        case 'd':
            return 0;
        case 'm':
        case 'c':
        case 'b':
            if (usrInput[CMD_SPC] == ' ') break;
            fprintf(stderr, "Invalid command format\n");
            return 1;
        default:
            fprintf(stderr, "Invalid command\n");
            return 1;
    }

    /* Check msg destinations */
    if (cmd == 'm' && !isalpha(usrInput[CMD_DEST])) {
        fprintf(stderr, "Invalid command format: Missing destination handle.\n");
        return 1;
    }

    /* Check multicast numDestinations */
    if (cmd == 'c' && (!isdigit(πusrInput[CMD_DEST]) || usrInput[CMD_DEST] < '2' || usrInput[CMD_DEST] > '9')) {
        fprintf(stderr, "Invalid command format, number of destinations must be between 2 and 9 (inclusive)\n");
        return 1;
    }

    return 0;

}

void processUsrInput(int clientSocket, char *handle) {

    uint8_t usrInput[MAX_MSG] = {0};

    fgets((char *) usrInput, MAX_MSG, stdin);

    /* Check for valid input */
    if (usrInput[0] != '%' || checkValidInput(usrInput)) {
        printf("$: ");
        fflush(stdout);
        return;
    }

    /* Replace the last '\n' with '\0' */
    usrInput[strnlen((char *) usrInput, MAX_MSG) - 1] = '\0';

    /* Parse given command */
    switch (tolower(usrInput[1])) {
        case 'm':
            packMessage(clientSocket, handle, usrInput);
            break;
        case 'c':
            packMulticast(clientSocket, handle, usrInput);
            break;
        case 'b':
            packBroadcast(clientSocket, handle, (char *) &(usrInput[3]));
            break;
        case 'l':
            reqList(clientSocket);
            break;
        case 'e':
            reqClose(clientSocket);
            break;
        case 'd':
            printf("hello!");
            break;
        default:
            fprintf(stderr, "Invalid command\n");
    }

    /* User prompt */
    printf("$: ");
    fflush(stdout);

}

void clientControl(int clientSocket, char *handle) {

    uint8_t pollSocket, run_client = 0;
    pollSet_t *pollSet = newPollSet();

    /* Add stdin and the client socket to the poll list */
    addToPollSet(pollSet, clientSocket);
    addToPollSet(pollSet, STDIN_FILENO);

    /* Run the client */
    while (!run_client) {

        /* Check poll */
        pollSocket = pollCall(pollSet, -1);

        /* Check for incoming packets and user input */
        if (pollSocket == clientSocket) run_client = processPacket(clientSocket, handle);
        else if (pollSocket == STDIN_FILENO) processUsrInput(clientSocket, handle);
    }

    /* Clean up */
    freePollSet(pollSet);

}
