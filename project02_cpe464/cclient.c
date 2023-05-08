
#include "cclient.h"

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

void checkSocketDisconnected(int bytesSent, int socket) {

    if (bytesSent < 1) {

        if (socket == -1) {

            /* Function inout error handling */
            fprintf(stderr, "\ncheckSocketDisconnected() input error! Terminating...\n");
            exit(EXIT_FAILURE);

        } else {

            /* Server disconnected from client */
            fprintf(stderr, "\nServer unexpectedly disconnected! \n");
            close(socket);
            exit(EXIT_FAILURE);

        }
    }
}

int sendToServer(int socket, uint8_t *sendBuf, int sendLen, uint8_t flag) {

    int bytesSent;

    /* Send the data as a packet with a header */
    bytesSent = sendPDU(socket, sendBuf, sendLen, flag);

    /* Make sure the server is still connected */
    checkSocketDisconnected(bytesSent, socket);

    return bytesSent;
}

int recvFromServer(int socket, uint8_t *recvBuff, uint16_t buffLen) {

    int bytesRecv = recvPDU(socket, recvBuff, buffLen);

    /* Make sure the server is still connected */
    checkSocketDisconnected(bytesRecv, socket);

    return bytesRecv;
}

void sendHandshake(int clientSocket, char *clientHandle) {

    uint8_t handleLen = (uint8_t) strnlen(clientHandle, 100);
    uint8_t sendLen = handleLen + 1;
    uint8_t sendBuf[MAX_USR];

    /* 1 Byte: Handle length */
    memcpy(sendBuf, &handleLen, 1);
    /* N Bytes: The client handle*/
    memcpy(sendBuf + 1, clientHandle, handleLen);

    /* Send the packet */
    sendToServer(clientSocket, sendBuf, sendLen, CONN_PKT);
}

void recvHandshake(int clientSocket) {

    uint8_t flag, recvBuff[MAX_USR];

    /* Wait for the server to send an ACK */
    printf("Waiting for server connection response...\n");
    recvFromServer(clientSocket, recvBuff, MAX_USR);

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
    char srcHandle[MAX_HDL], msg[MAX_USR];

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

    uint8_t recvBuff[MAX_USR] = {0};

    /* Receive a new packet */
    recvFromServer(socket, recvBuff, MAX_USR);

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
            recvClientList(socket, recvBuff, MAX_USR);
    }

    return 0;
}

int chopUsrMessage(char usrMessage[], char messages[][MAX_MSG]) {

    uint8_t i, packetLen;
    uint16_t offset = 0;

    for (i = 0; i < MAX_PKTS; i++) {
        packetLen = strnlen(&usrMessage[offset], MAX_MSG - 1);

        if (packetLen < 1) break;

        snprintf(messages[i], MAX_MSG, "%s", &usrMessage[offset]);

        offset += packetLen;
    }

    return i;
}

void packMessage(int socket, char *clientHandle, char usrInput[]) {

    uint8_t sendBuff[MAX_USR] = {0};
    uint8_t i, dataBuffLen = 0, strLen, numDests = 1, numPackets;
    char *dstHandle = NULL, usrMsg[MAX_USR], messages[MAX_PKTS][MAX_MSG];

    /* Get the handle */
    dstHandle = strtok((char *) &(usrInput[3]), " ");

    strLen = (uint8_t) strnlen(dstHandle, MAX_HDL - 1) + 1;
    snprintf(usrMsg, MAX_USR, "%s", (char *) &(usrInput[strLen + 3]));

    numPackets = chopUsrMessage(usrMsg, messages);

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

    /* Send as many packets as necessary depending on message length */
    for (i = 0; i < numPackets; i++) {

        /* 1 Byte: Message length */
        strLen = (uint8_t) strnlen(messages[i], MAX_MSG) + 1;
        /* N Bytes: The message with null terminator */
        memcpy(&sendBuff[dataBuffLen], messages[i], strLen);

        /* Send the packet! */
        sendToServer(socket, sendBuff, dataBuffLen + strLen, MESSAGE_PKT);
    }

}

void packBroadcast(int socket, char *handle, char *msg) {

    uint8_t sendBuff[MAX_USR], numPackets, strLen, i;
    char messages[MAX_PKTS][MAX_MSG];

    /* Add handle length and handle */
    uint16_t handleLen = strnlen(handle, MAX_HDL);
    memcpy(sendBuff, &handleLen, 1);
    memcpy(sendBuff + 1, handle, handleLen++);

    /* Parse the message into equally sized message packets */
    numPackets = chopUsrMessage(msg, messages);

    /* If the user didn't provide a message, just send a null terminator */
    if (numPackets == 0) {

        memcpy(sendBuff + handleLen++, "\0", 1);
        sendToServer(socket, sendBuff, handleLen, BROADCAST_PKT);
    }

    /* Send as many packets as necessary depending on message length */
    for (i = 0; i < numPackets; i++) {

        /* Add the message */
        strLen = (int) strnlen(messages[i], MAX_MSG) + 1;
        memcpy(&sendBuff[handleLen], messages[i], strLen);

        /* Send the packet! */
        sendToServer(socket, sendBuff, handleLen + strLen, BROADCAST_PKT);

    }
}

void packMulticast(int socket, char *handle, char usrInput[]) {

    uint8_t i, sendBuff[MAX_USR] = {0}, handleLen, numDestinations, numPackets, strLen;
    uint16_t msgOffset = 5, buffLen = 1;
    char *dstHandle = NULL, messages[MAX_PKTS][MAX_MSG];

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
            fprintf(stderr, "Invalid command format: Requested %d destinations but only %d given.\n",
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

    numPackets = chopUsrMessage(&usrInput[msgOffset], messages);

    /* Send as many packets as necessary depending on message length */
    for (i = 0; i < numPackets; i++) {

        /* 1 Byte: Message length */
        strLen = (uint8_t) strnlen(messages[i], MAX_MSG) + 1;
        /* N Bytes: The message with null terminator */
        memcpy(&sendBuff[buffLen], messages[i], strLen);

        /* Send the packet! */
        sendToServer(socket, sendBuff, buffLen + strLen, MULTICAST_PKT);
    }
}

void reqList(int socket) {

    /* Send a close connection request packet */
    sendToServer(socket, NULL, 0, REQ_LIST_PKT);

}

void reqClose(int socket) {

    /* Send a close connection request packet */
    sendToServer(socket, NULL, 0, REQ_EXIT_PKT);

}

int checkValidInput(char usrInput[]) {

    /* Check for proper formatting */
    uint8_t cmd = tolower(usrInput[USR_CMD]);

    /* There should always be some form of whitespace after each command */
    if (!isspace(usrInput[CMD_SPC]) && usrInput[CMD_SPC] != '\0') {
        fprintf(stderr, "Invalid command format\n");
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
    if (cmd == 'c' && (!isdigit(usrInput[CMD_DEST]) || usrInput[CMD_DEST] < '2' || usrInput[CMD_DEST] > '9')) {
        fprintf(stderr, "Invalid command format, number of destinations must be between 2 and 9 (inclusive)\n");
        return 1;
    }

    return 0;

}

uint16_t readFromStdin(char usrInput[]) {

    uint16_t maxLen = MAX_USR + 1;
    char tempStr[maxLen];
    uint16_t strLen;

    /* Get the user's input */
    fgets((char *) tempStr, maxLen, stdin);

    /* Remove anything we didn't grab from stdin */
    fflush(stdin);

    /* Check the length of the input */
    strLen = strnlen(tempStr, maxLen);

    /* Replace '\n' with '\0' */
    tempStr[strLen - 1] = '\0';

    /* Check input length */
    if (strLen > MAX_USR) {
        /* Error message */
        fprintf(
                stderr,
                "Input was longer than %d characters (including null terminator), "
                "the message will be truncated.\n",
                MAX_USR
        );

        /* Truncate the message */
        tempStr[MAX_USR - 1] = '\0';
    }

    /* Copy the message into the given buffer */
    strncpy(usrInput, tempStr, strLen);

    return strLen;
}

void processUsrInput(int clientSocket, char *handle) {

    char usrInput[MAX_USR] = {0};

    /* Read user input */
    readFromStdin(usrInput);

    /* Check for valid input */
    if (usrInput[0] != '%' || checkValidInput(usrInput)) {
        printf("$: ");
        fflush(stdout);
        return;
    }

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
        pollSocket = pollCall(pollSet, POLL_WAIT_FOREVER);

        /* Check for incoming packets and user input */
        if (pollSocket == clientSocket) run_client = processPacket(clientSocket, handle);
        else if (pollSocket == STDIN_FILENO) processUsrInput(clientSocket, handle);
    }

    /* Clean up */
    freePollSet(pollSet);

}
