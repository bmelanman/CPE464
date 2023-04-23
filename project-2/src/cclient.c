
#include "cclient.h"

int main(int argc, char *argv[]) {

    int socket;

    /* Check input arguments */
    checkArgs(argc, argv);

    /* Set up the TCP Client socket with the handle, server name, and port  */
    socket = clientInitTCP(argv[1], argv[2], argv[3]);

    /* Run the client */
    clientControl(socket, argv[1]);

    /* Clean up */
    close(socket);

    return 0;
}

void clientControl(int clientSocket, char *handle) {

    uint8_t pollSocket, run_client = 0;

    /* Add stdin and the client socket to the poll list */
    addToPollSet(clientSocket);
    addToPollSet(STDIN_FILENO);

    /* Run the client */
    while (!run_client) {

        /* Check poll */
        pollSocket = pollCall(0);

        /* Check for incoming packets and user input */
        if (pollSocket == clientSocket) run_client = processUsrInput(clientSocket, handle);
        else if (pollSocket == STDIN_FILENO) processPacket(clientSocket);
    }

}

int clientInitTCP(char *handle, char *server_name, char *server_port) {

    int sock, err;
    struct addrinfo *info, hints = {
            0,
            AF_INET6,
            SOCK_STREAM,
            IPPROTO_IP,
            0,
            "",
            NULL,
            NULL
    };

    printf(
            "Server Connected!\n"
            "Your Handle: %s\n"
            "Server Name: %s\n"
            "Server Port: %s\n"
            "\n",
            handle, server_name, server_port
    );

    /* Get hostname of server_net_init */
    err = getaddrinfo(server_name, server_port, &hints, &info);
    if (err != 0) {
        fprintf(stderr, "getaddrinfo() err #%d:\n", err);
        fprintf(stderr, "%s\n", gai_strerror(err));
        return -1;
    }

    /* Open a new socket */
    sock = socket(info->ai_family, info->ai_socktype, info->ai_protocol);
    if (sock == -1) {
        fprintf(stderr, "Couldn't open socket!\n");
        return -1;
    }

    /* Connect the socket */
    err = connect(sock, info->ai_addr, info->ai_addrlen);
    if (err != 0) {
        fprintf(stderr, "Could not connect to server_net_init!\n");
        return -1;
    }

    /* Clean up */
    freeaddrinfo(info);

    return sock;
}

int processUsrInput(int socket, char *handle) {

    int exit_flag = 1;
    char buff[MAXBUF] = "";
    char *strBuff = NULL;

    while (exit_flag) {

        printf("$: ");

        fgets(buff, MAXBUF, stdin);

        /* Check for % at the beginning of each command */
        if (buff[0] != '%') {
            fprintf(stderr, "Invalid command format\n");
            continue;
        }

        /* Parse given command */
        switch (tolower(buff[1])) {
            case 'm':
                /* Get the handle */
                strBuff = strtok(&(buff[2]), " ");
                /* Send the message */
                packMessage(0, handle, strBuff, strtok(NULL, " "));
                break;
            case 'b':
                broadcast();
                break;
            case 'c':
                multicast();
                break;
            case 'l':
                list();
                break;
            case 'e':
                exit_flag = 0;
                break;
            default:
                fprintf(stderr, "Invalid command\n");
                continue;
        }

    }

    /* Clean up */
    close(socket);

    return 0;
}

void processPacket(int socket) {

    printf("Process packet on socket %d", socket);

}

void packMessage(int socket, char *clientHandle, char *dstHandle, char *usrMsg) {

    uint8_t dataBuff[MAXBUF];
    uint8_t dataBuffLen = 0;

    uint8_t flag = 5;
    uint8_t numDests = 1;

    /* Lengths of each string sent */
    uint8_t srcHandleLen = (uint8_t) strlen(clientHandle);
    uint8_t dstHandleLen = (uint8_t) strlen(dstHandle);
    uint8_t msgLen = (uint8_t) strlen(usrMsg);

    /* 2 Bytes: Message packet length */
    memcpy(dataBuff, &msgLen, 2);
    dataBuffLen += 2;
    /* 1 Byte: Message packet flag */
    memcpy(dataBuff + dataBuffLen, &flag, 1);
    ++dataBuffLen;

    /* 1 Byte: Client handle length */
    memcpy(dataBuff + dataBuffLen, &srcHandleLen, 1);
    ++dataBuffLen;
    /* N Bytes: Client handle */
    memcpy(dataBuff + dataBuffLen, clientHandle, srcHandleLen);
    dataBuffLen += srcHandleLen;

    /* 1 Byte: Number of destinations */
    memcpy(dataBuff + dataBuffLen, &numDests, 1);
    ++dataBuffLen;

    /* 1 Byte: Destination handle length */
    memcpy(dataBuff + dataBuffLen, &dstHandleLen, 1);
    ++dataBuffLen;
    /* N Bytes: Destination handle */
    memcpy(dataBuff + dataBuffLen, &dstHandle, dstHandleLen);
    dataBuffLen += dstHandleLen;

    /* N Bytes: The message */
    memcpy(dataBuff + dataBuffLen, &usrMsg, msgLen);
    dataBuffLen += msgLen;

    /* Send the packet! */
    sendToServer(socket, dataBuff, dataBuffLen);

}

void broadcast(void) {
    printf("broadcast\n");
}

void multicast(void) {
    printf("multicast\n");
}

void list(void) {
    printf("list\n");
}

int sendToServer(int socket, uint8_t *sendBuf, int sendLen) {

    int sent;

    printf(
            "string len: %d (including null)\n",
            sendLen
    );

    /* Send the data as a packet with a header */
    sent = sendPDU(socket, sendBuf, sendLen);

    printf("Amount of data sent is: %d\n", sent);

    return sent;
}

int readFromStdin(uint8_t *buffer) {
    int aChar, inputLen = 0;

    /* Important you don't input more characters than you have space for */
    buffer[inputLen] = '\0';

    printf("Enter data: ");

    /* Reach each character until we reach \n */
    while (inputLen < (MAXBUF - 1) && (aChar = getchar()) != '\n') {

        if (aChar != '\n') {
            buffer[inputLen++] = aChar;
        }

    }

    /* Null terminate the string */
    buffer[inputLen++] = '\0';

    return inputLen;
}

void checkArgs(int argc, char *argv[]) {

    /* Check command line arguments  */
    if (argc != 4) {
        fprintf(stderr, "Usage: ./%s handle serverName serverPort \n", argv[0]);
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
}
