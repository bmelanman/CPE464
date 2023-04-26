
#include "cclient.h"

/* Private prototypes */
void checkArgs(int argc, char *argv[]);

int clientInitTCP(char *handle, char *server_name, char *server_port);

void clientControl(int clientSocket, char *handle);

void processUsrInput(int socket, char *handle);

void sendHandshake(int clientSocket, char *clientHandle);

void recvClientList(int clientSocket, uint8_t dataBuff[], int buffLen);

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

int clientInitTCP(char *handle, char *server_name, char *server_port) {

    int clientSocket, err, bytes, flag;
    uint8_t dataBuff[MAXBUF];
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
    bytes = recvPDU(clientSocket, dataBuff, MAXBUF);
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
        fprintf(stderr, "Server disconnected during initialization, terminating...\n");
        close(clientSocket);
        exit(EXIT_FAILURE);
    }

}

void printMessage(uint8_t dataBuff[]) {

    char *srcHandle, *msg;
    int dataLen, offset = 4;

    /* Get the length of the src handle */
    dataLen = dataBuff[PDU_SRC_LEN_IDX] + 1;

    /* Get the src handle */
    srcHandle = strndup((char *) &dataBuff[offset], dataLen++);

    /* Get the length of the dst handle */
    offset = PDU_SRC_LEN_IDX + dataLen;
    dataLen = dataBuff[offset];

    /* Get the message */
    offset += dataLen + 2;
    msg = strdup((char *) &dataBuff[offset]);

    printf("\n%s: %s\n", srcHandle, msg);
    printf("$: ");
    fflush(stdout);

    free(srcHandle);
    free(msg);
}

int processPacket(int socket) {

    uint8_t recvBuff[MAXBUF] = {0};
    int recvLen;

    /* Receive a new packet */
    recvLen = recvPDU(socket, recvBuff, MAXBUF);

    /* Check for server connection */
    if (recvLen == 0) {
        printf("Server has disconnected!\n");
        return 1;
    }

    switch (recvBuff[PDU_FLAG]) {
        case 5: /* Message received packet */
            printMessage(recvBuff);
            break;
        case 7: /* Message error packet */
            printf("\nClient with handle %s does not exist.\n$: ",
                   strndup((char *) (recvBuff + 4), recvBuff[PDU_SRC_LEN_IDX])
            );
            fflush(stdout);
            break;
        case 9: /* Exit ACK packet */
            printf("Exit request approved, shutting down...\n");
            return 1;
        case 11:
            recvClientList(socket, recvBuff, MAXBUF);
    }

    return 0;

}

void recvClientList(int clientSocket, uint8_t dataBuff[], int buffLen) {

    int bytes, flag = 12;
    uint32_t numClients = ntohl(*((uint32_t *) &dataBuff[PDU_HEADER_LEN]));

    printf("\nNumber of clients: %u\n$: ", numClients);
    fflush(stdout);

    while (flag == 12) {

        bytes = recvPDU(clientSocket, dataBuff, buffLen);

        /* Get the first  */
        if (bytes == 0) {
            printf("Server has disconnected!\n");
            return;
        }
    }

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
        if (pollSocket == clientSocket) run_client = processPacket(clientSocket);
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

void packMessage(int socket, char *clientHandle, char *dstHandle, char *usrMsg) {

    uint8_t dataBuff[MAXBUF] = {0};
    uint8_t dataBuffLen = 0, strLen;
    uint8_t numDests = 1;

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
    memcpy(dataBuff + (dataBuffLen++), &strLen, 1);
    /* N Bytes: Client handle */
    memcpy(dataBuff + dataBuffLen, clientHandle, strLen);
    dataBuffLen += strLen;

    /* 1 Byte: Number of destinations */
    memcpy(dataBuff + (dataBuffLen++), &numDests, 1);

    /* 1 Byte: Destination handle length */
    strLen = (int) strlen(dstHandle);
    memcpy(dataBuff + (dataBuffLen++), &strLen, 1);
    /* N Bytes: Destination handle */
    memcpy(dataBuff + dataBuffLen, dstHandle, strLen);
    dataBuffLen += strLen;

    /* 1 Byte: Message length */
    if (usrMsg == NULL) strLen = 0;
    else strLen = (int) strlen(usrMsg);
    memcpy(dataBuff + (dataBuffLen++), &strLen, 1);
    /* N Bytes: The message */
    memcpy(dataBuff + dataBuffLen, usrMsg, strLen);
    dataBuffLen += strLen;

    /* Send the packet! */
    sendToServer(socket, dataBuff, dataBuffLen, MESSAGE_PKT);

}

void packBroadcast(void) {
    printf("packBroadcast\n");
}

void packMulticast(void) {
    printf("packMulticast\n");
}

void reqList(int socket) {

    /* Send a close connection request packet */
    sendToServer(socket, NULL, 0, REQ_LIST_PKT);

}

void reqClose(int socket) {

    /* Send a close connection request packet */
    sendToServer(socket, NULL, 0, REQ_EXIT_PKT);

}

void processUsrInput(int socket, char *handle) {

    uint8_t usrInput[MAXBUF] = {0}, strLen;
    char *strBuff = NULL;

    fgets((char *) usrInput, MAXBUF, stdin);

    usrInput[strnlen((char *) usrInput, MAXBUF) - 1] = '\0';

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
            /* Get the handle */
            strBuff = strtok((char *) &(usrInput[3]), " ");
            strLen = strnlen(strBuff, MAXBUF);

            /* Send the message */
            packMessage(socket, handle, strBuff, (char *) &(usrInput[4 + strLen]));
            break;
        case 'c':
            packMulticast();
            break;
        case 'b':
            packBroadcast();
            break;
        case 'l':
            reqList(socket);
            break;
        case 'e':
            reqClose(socket);
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
