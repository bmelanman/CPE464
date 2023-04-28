
#include "cclient.h"

/* Private prototypes */
void checkArgs(int argc, char *argv[]);

int clientInitTCP(char *handle, char *server_name, char *server_port);

void clientControl(int clientSocket, char *handle);

void processUsrInput(int clientSocket, char *handle);

/* Functions */
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

void sendHandshake(int clientSocket, char *clientHandle) {

    int bytes, handleLen = (int) strnlen(clientHandle, 100);
    uint8_t dataBuff[101];

    /* 1 Byte: Handle length */
    memcpy(dataBuff, &handleLen, 1);
    /* N Bytes: The client handle*/
    memcpy(dataBuff + 1, clientHandle, handleLen);

    /* Send the packet */
    bytes = sendPDU(clientSocket, dataBuff, handleLen + 1, CONN_PKT);
    if (bytes < 1) {
        fprintf(stderr, "\nServer disconnected during initialization, terminating...\n");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

}

int clientInitTCP(char *handle, char *server_name, char *server_port) {

    int clientSocket, err, bytes, flag;
    uint8_t dataBuff[MAX_BUF];
    struct addrinfo *info, hints = {
            0,
            AF_INET6,
            SOCK_STREAM,
            IPPROTO_IP,
            0,
            server_name,
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

    /* Wait for the server to send an ACK */
    printf("Waiting for server connection response...\n");
    bytes = recvPDU(clientSocket, dataBuff, MAX_BUF);
    if (bytes < 1) {
        fprintf(stderr, "Server disconnected during initialization, terminating...\n");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

    /* Get the packet flag */
    flag = dataBuff[PDU_FLAG];

    if (flag == 2) {
        /* Successful connection */
        printf("Acknowledgement received, server connection successful!\n");
    } else if (flag == 3) {
        /* Server declined due to invalid handle */
        fprintf(stderr, "Server declined connection due to an invalid handle, please try again.\n");
        close(clientSocket);
        exit(EXIT_FAILURE);
    } else {
        /* Safety error handling */
        fprintf(stderr, "Packet with unknown flag \'%d\' received, terminating...", flag);
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

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
    snprintf(msg, MAX_MSG, "%s", &dataBuff[offset]);

    printf("\n%s: %s\n", srcHandle, msg);
    printf("$: ");
    fflush(stdout);

}

void processBroadcast(uint8_t dataBuff[]) {

    /* Unpack the packet */
    int handleLen = dataBuff[1];
    char handle[MAX_HDL];
    memcpy(handle, &dataBuff[2], handleLen);
    handle[handleLen] = '\0';
    char *msg = strndup((char *) &dataBuff[handleLen + 2], MAX_MSG);

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
    snprintf(msg, MAX_MSG + MAX_HDL, "\n%s: %s\n", srcHandle, (char *) &dataBuff[offset]);

    printf("%s$: ", msg);
    fflush(stdout);
}

void recvClientList(int clientSocket, uint8_t dataBuff[], int buffLen) {

    int bytes, handleLen;
    uint32_t numClients = ntohl(*((uint32_t *) &dataBuff[1]));
    char handle[MAX_HDL];

    printf("\nNumber of clients: %u\n", numClients);
    fflush(stdout);

    while (1) {

        /* Get the next handle packet */
        bytes = recvPDU(clientSocket, dataBuff, buffLen);

        if (bytes == 0) {
            printf("\nServer has disconnected!\n");
            return;
        }

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
    int recvLen;

    /* Receive a new packet */
    recvLen = recvPDU(socket, recvBuff, MAX_BUF);

    /* Check for server connection */
    if (recvLen == 0) {
        printf("\nServer has disconnected!\n");
        return 1;
    }

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

int sendToServer(int socket, uint8_t *sendBuf, int sendLen, uint8_t flag) {

    int sent;

    /* Send the data as a packet with a header */
    sent = sendPDU(socket, sendBuf, sendLen, flag);

    if (sent < 1) fprintf(stderr, "Err: Server has disconnected from client!\n");

    return sent;
}

void packMessage(int socket, char *clientHandle, uint8_t *usrInput) {

    uint8_t sendBuff[MAX_BUF] = {0};
    uint8_t dataBuffLen = 0, strLen, numDests = 1;
    char *dstHandle = NULL, *usrMsg = NULL;

    /* Get the handle */
    dstHandle = strtok((char *) &(usrInput[3]), " ");
    strLen = strnlen(dstHandle, MAX_HDL - 1) + 1;
    usrMsg = (char *) &(usrInput[strLen + 3]);

    /* Lengths of each string sent */
    if (clientHandle == NULL) {
        fprintf(stderr, "Error, client handle is NULL!\n");
        exit(EXIT_FAILURE);
    } else if (dstHandle == NULL) {
        fprintf(stderr, "Invalid command format: A destination handle is required.\n");
        return;
    }

    /* 1 Byte: Client handle length */
    strLen = (int) strlen(clientHandle);
    memcpy(&sendBuff[dataBuffLen++], &strLen, 1);
    /* N Bytes: Client handle */
    memcpy(&sendBuff[dataBuffLen], clientHandle, strLen);
    dataBuffLen += strLen;

    /* 1 Byte: Number of destinations */
    memcpy(&sendBuff[dataBuffLen++], &numDests, 1);

    /* 1 Byte: Destination handle length */
    strLen = (int) strlen(dstHandle);
    memcpy(&sendBuff[dataBuffLen++], &strLen, 1);
    /* N Bytes: Destination handle */
    memcpy(&sendBuff[dataBuffLen], dstHandle, strLen);
    dataBuffLen += strLen;

    /* 1 Byte: Message length */
    if (usrMsg == NULL) strLen = 0;
    else strLen = (int) strlen(usrMsg);
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

    uint8_t sendBuff[MAX_BUF] = {0}, handleLen, buffLen = 1, numDestinations;
    char *dstHandle = NULL, *msg = NULL;

    /* Check for a valid number of destinations */
    if (!isnumber(usrInput[3]) || usrInput[3] < '2' || usrInput[3] > '9') {

        printf("Invalid command format, number of destinations must be between 2 and 9 (inclusive)\n$: ");
        fflush(stdout);
        return;
    }

    /* Add source handle and length */
    handleLen = strnlen(handle, MAX_HDL);
    memcpy(sendBuff, &handleLen, 1);
    memcpy(&sendBuff[buffLen], handle, handleLen);
    buffLen += handleLen;

    /* Add the number of destinations */
    numDestinations = strtol(strtok((char *) &usrInput[3], " "), NULL, 10);
    memcpy(&sendBuff[buffLen++], &numDestinations, 1);

    for (int i = 0; i < numDestinations; i++) {

        dstHandle = strtok(NULL, " ");
        handleLen = strnlen(dstHandle, MAX_HDL);
        memcpy(&sendBuff[buffLen++], &handleLen, 1);
        memcpy(&sendBuff[buffLen], dstHandle, handleLen);
        buffLen += handleLen;

    }

    /* Add the message */
    msg = strtok(NULL, " ");
    if (msg == NULL) handleLen = 0;
    else handleLen = strnlen(msg, MAX_MSG);
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

void processUsrInput(int clientSocket, char *handle) {

    uint8_t usrInput[MAX_MSG] = {0};

    fgets((char *) usrInput, MAX_MSG, stdin);

    usrInput[strnlen((char *) usrInput, MAX_MSG) - 1] = '\0';

    /* Check for % at the beginning of each command */
    if (usrInput[0] != '%') {
        fprintf(stderr, "Invalid command format\n");
        goto prompt;
    }

    /* Check for proper formatting */
    switch (tolower(usrInput[1])) {
        case 'm':
        case 'c':
        case 'b':
            if (usrInput[2] != ' ') {
                fprintf(stderr, "Invalid command format\n");
                goto prompt;
            }
            break;
        case 'l':
        case 'e':
        case 'd':
            if (usrInput[2] != '\0' && usrInput[2] != '\n') {
                fprintf(stderr, "Invalid command format\n");
                goto prompt;
            }
            break;
        default:
            fprintf(stderr, "Invalid command\n");
            goto prompt;
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

    prompt:

    /* User prompt */
    printf("$: ");
    fflush(stdout);

}
