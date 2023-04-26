
#include "server.h"

static volatile int shutdownServer = 0;

void intHandler(void) {
    printf("\n\nShutting down server...\n");
    shutdownServer = 1;
}

int main(int argc, char *argv[]) {
    int mainServerSocket;
    int portNumber;

    /* Check inout arguments */
    portNumber = checkArgs(argc, argv);

    /* Catch for ^C */
    signal(SIGINT, (void (*)(int)) intHandler);

    /* Set up poll */
    newPollSet();

    /* Create the server socket */
    mainServerSocket = tcpServerSetup(portNumber);

    /* Run the server */
    serverControl(mainServerSocket);

    /* Close the main socket */
    close(mainServerSocket);

    return 0;
}

int checkArgs(int argc, char *argv[]) {

    /* Checks args and returns port number */
    int portNumber = 0;

    /* Check for optional port number */
    if (argc > 2) {
        fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
        exit(EXIT_FAILURE);

    } else if (argc == 2) {
        portNumber = (int) strtol(argv[1], NULL, 10);
    }

    return portNumber;
}

int tcpServerSetup(int serverPort) {
    /* Hugh Smith - April 2017 */

    int mainServerSocket;
    struct sockaddr_in6 serverAddress;
    socklen_t serverAddressLen = sizeof(serverAddress);

    mainServerSocket = socket(AF_INET6, SOCK_STREAM, 0);
    if (mainServerSocket < 0) {
        perror("socket call");
        exit(EXIT_FAILURE);
    }

    memset(&serverAddress, 0, sizeof(struct sockaddr_in6));
    serverAddress.sin6_family = AF_INET6;
    serverAddress.sin6_addr = in6addr_any;
    serverAddress.sin6_port = htons(serverPort);

    /* Bind the name (address) to a port */
    if (bind(mainServerSocket, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        perror("bind call");
        exit(EXIT_FAILURE);
    }

    /* Get the port name and print it out */
    if (getsockname(mainServerSocket, (struct sockaddr *) &serverAddress, &serverAddressLen) < 0) {
        perror("getsockname call");
        exit(EXIT_FAILURE);
    }

    if (listen(mainServerSocket, LISTEN_BACKLOG) < 0) {
        perror("listen call");
        exit(EXIT_FAILURE);
    }

    printf("Server Port Number %d \n", ntohs(serverAddress.sin6_port));

    return mainServerSocket;
}

static char *getIPAddrStr(unsigned char *ipAddr) {
    /* Hugh Smith - April 2017 */

    static char ipStr[INET6_ADDRSTRLEN];

    if (ipAddr != NULL) {
        inet_ntop(AF_INET6, ipAddr, ipStr, sizeof(ipStr));
    } else {
        strncpy(ipStr, "(IP not found)", 15);
    }

    return ipStr;
}

int tcpAccept(int mainServerSocket) {
    /* Hugh Smith - April 2017 */

    struct sockaddr_in6 clientAddress;
    int clientSocket, clientAddressSize = sizeof(clientAddress);

    clientSocket = accept(mainServerSocket, (struct sockaddr *) &clientAddress, (socklen_t *) &clientAddressSize);

    if (clientSocket < 0) {
        perror("accept call");
        exit(EXIT_FAILURE);
    }

    printf(
            "Client accepted!\n"
            "Client IP: %s\n"
            "Client Port Number: %d\n",
            getIPAddrStr(clientAddress.sin6_addr.s6_addr),
            ntohs(clientAddress.sin6_port)
    );

    return clientSocket;
}

void addNewSocket(serverTable_t *serverTable, int socket) {

    int clientSocket = tcpAccept(socket);

    addToPollTable(serverTable, clientSocket);
}

void processNewClient(int clientSocket, uint8_t dataBuff[], serverTable_t *serverTable) {

    int bytesSent;

    /* Get the length of the handle and add 1 byte for the null terminator */
    int handleLen = dataBuff[PDU_SRC_LEN_IDX];
    char clientHandle[handleLen + 1];

    /* Grab the handle */
    memcpy(clientHandle, dataBuff + PDU_SRC_LEN_IDX + 1, handleLen);
    clientHandle[handleLen++] = '\0';

    /* Check for duplicate headers */
    bytesSent = addClient(serverTable, clientSocket, clientHandle);

    /* Add the client to the server table */
    if (bytesSent == 0) {

        /* Accept the client connection */
        bytesSent = sendPDU(clientSocket, NULL, 0, CONN_ACK_PKT);

        if (bytesSent < 1) {
            /* Client disconnected before the server could respond */
            removeClient(serverTable, clientHandle);
        }

    } else {

        /* Decline the connection */
        sendPDU(clientSocket, NULL, 0, CONN_ERR_PKT);
        removeFromPollSet(serverTable->pollSet, clientSocket);

    }
}

void sendToAll(serverTable_t *serverTable, int clientSocket, int flag, uint8_t sendBuff[], int pduLen) {

    /* Variables are initialized for broadcast packets */
    int bytesSent, sock = clientSocket;
    uint8_t handleLen, sendLen = pduLen;
    char *handle = NULL;

    /* Send each handle as in a packet 12 */
    for (int i = 0; i < serverTable->arrCap; ++i) {

        /* Get the next handle */
        handle = serverTable->handleArr[i];

        /* Check for valid handles */
        if (handle == NULL || handle[0] == '\0') continue;

        /* Check if we need to switch to handshake packets */
        if (flag == HDL_LIST_PKT) {

            /* Pack a packet 12 */
            handleLen = (int) strnlen(handle, MAX_HANDLE_LEN);
            memcpy(sendBuff, &handleLen, 1);
            memcpy(&sendBuff[1], handle, handleLen);

            sendLen = ++handleLen;
            sock = i;

        }

        bytesSent = sendPDU(sock, sendBuff, sendLen, flag);
        if (bytesSent < 1) {
            printf("Client unexpectedly disconnected\n");
            removeClientSocket(serverTable, clientSocket);
            return;

        }
    }
}

void routeBroadcast(serverTable_t *serverTable, uint8_t dataBuff[], int pduLen) {

//    int bytes;
//    char *handle;

    sendToAll(serverTable, -1, BROADCAST_PKT, dataBuff, pduLen);

//    /* Cycle through all sockets to find valid ones */
//    for (int sock = 3; sock < serverTable->arrCap; sock++) {
//
//        /* Get the handle at the socket index */
//        handle = serverTable->handleArr[sock];
//
//        /* Check for a valid handle */
//        if (handle == NULL || handle[0] == '\0') continue;
//
//        /* Send the packet to the client */
//        bytes = sendPDU(sock, &dataBuff[3], pduLen, BROADCAST_PKT);
//        if (bytes < 1) {
//            printf("Client unexpectedly disconnected\n");
//            removeClientSocket(serverTable, sock);
//        }
//    }

}

void routeMessage(serverTable_t *serverTable, uint8_t dataBuff[], int pduLen) {

    char srcHandle[MAX_HDL], dstHandle[MAX_HDL];
    int srcSocket, dstSocket;

    uint8_t handleLen = dataBuff[PDU_HEADER_LEN] + 1, offset = PDU_HEADER_LEN + 1;

    /* Get the source handle */
    snprintf(srcHandle, handleLen, "%s", &dataBuff[offset]);

    /* Get the destination handle */
    offset += handleLen;
    handleLen = dataBuff[offset++] + 1;
    snprintf(dstHandle, handleLen, "%s", &dataBuff[offset]);

    dstSocket = getClient(serverTable, dstHandle);

    /* Check if the destination client exists in our serverTable */
    if (dstSocket == -1) {
        /* Get the socket of the source client */
        srcSocket = getClient(serverTable, srcHandle);

        /* Decrement offset so that it will point to the dst handle length */
        offset--;

        /* Send an error packet containing the length of the handle + the handle itself */
        sendPDU(srcSocket, &dataBuff[offset], handleLen + 1, DST_ERR_PKT);

        return;
    }

    /* Send the message packet to the destination client with the chat header removed (it will be added again by sendPDU) */
    sendPDU(dstSocket, &dataBuff[3], pduLen, MESSAGE_PKT);

    printf("Packet of length %d sent from \"%s\" to \"%s\"\n", pduLen, srcHandle, dstHandle);

}

void routeMulticast(serverTable_t *serverTable, uint8_t dataBuff[], int pduLen) {
//todo:
}

void sendClose(serverTable_t *serverTable, int clientSocket) {

    int bytesSent = sendPDU(clientSocket, NULL, 0, ACK_EXIT_PKT);

    if (bytesSent > 0) printf("Close request acknowledged, sent %d bytes\n", bytesSent);

    removeClientSocket(serverTable, clientSocket);
}

void sendList(serverTable_t *serverTable, int clientSocket) {

    uint8_t bytesSent, sendBuff[MAX_BUF], handleLen;
    uint32_t numClients = htonl(serverTable->size);
//    char *handle = NULL;

    /* Send a packet 11 containing the number of clients in the serverTable */
    memcpy(sendBuff, &numClients, 4);
    bytesSent = sendPDU(clientSocket, sendBuff, 4, ACK_LIST_PKT);
    if (bytesSent < 1) {
        printf("Client unexpectedly disconnected\n");
        removeClientSocket(serverTable, clientSocket);
        return;
    }

    sendToAll(serverTable, clientSocket, HDL_LIST_PKT, sendBuff, 0);

//    /* Send each handle as in a packet 12 */
//    for (int sock = 3; sock < serverTable->arrCap; ++sock) {
//
//        /* Get the next handle */
//        handle = serverTable->handleArr[sock];
//
//        /* Check for valid handles */
//        if (handle == NULL || handle[0] == '\0') continue;
//
//        /* Pack and send */
//        handleLen = strnlen(handle, MAX_HANDLE_LEN);
//        memcpy(sendBuff, &handleLen, 1);
//        memcpy(&sendBuff[1], handle, handleLen);
//
//        bytesSent = sendPDU(clientSocket, sendBuff, 1 + handleLen, HDL_LIST_PKT);
//        if (bytesSent < 1) {
//            printf("Client unexpectedly disconnected\n");
//            removeClientSocket(serverTable, clientSocket);
//            return;
//
//        }
//    }

    /* Finish with a packet 13 */
    bytesSent = sendPDU(clientSocket, NULL, 0, FIN_LIST_PKT);
    if (bytesSent < 1) {
        printf("Client unexpectedly disconnected\n");
        removeClientSocket(serverTable, clientSocket);
        return;
    }

}

void processClient(int clientSocket, serverTable_t *serverTable) {

    uint8_t recvBuffer[MAX_BUF];
    int messageLen;

    /* Now get the data from the client_socket */
    messageLen = recvPDU(clientSocket, recvBuffer, MAX_BUF);

    /* If the message length is -1, the client has disconnected */
    if (messageLen == 0) {
        removeClientSocket(serverTable, clientSocket);
    }

    switch (recvBuffer[PDU_FLAG]) {
        case 1:     /* New client handshake */
            processNewClient(clientSocket, recvBuffer, serverTable);
            break;
        case 4:   /* Broadcast packet */
            routeBroadcast(serverTable, recvBuffer, messageLen);
            break;
        case 5:     /* Message packet */
            routeMessage(serverTable, recvBuffer, messageLen);
            break;
        case 6:   /* Multicast packet */
            routeMulticast(serverTable, recvBuffer, messageLen);
            break;
        case 8:     /* Exit request */
            sendClose(serverTable, clientSocket);
            break;
        case 10:    /* List request */
            sendList(serverTable, clientSocket);
            break;
        default:    /* Invalid packet */
            /* Received invalid/corrupt packet, but client has not disconnected */
            printf("Unknown packet of length %d received with flag %d.\n", messageLen, recvBuffer[PDU_FLAG]);
    }

}

void serverControl(int mainServerSocket) {

    int pollSocket;
    serverTable_t *serverTable = newServerTable(1);

    /* Add the main server socket to the list of sockets to poll */
    addToPollTable(serverTable, mainServerSocket);
    addToPollTable(serverTable, STDIN_FILENO);

    while (!shutdownServer) {

        /* Call poll() */
        pollSocket = callTablePoll(serverTable, -1);

        /* No new packets (useful for debugging when 'poll() interrupted' errors can occur more frequently) */
        if (pollSocket == -1) continue;

        /* Check for new sockets */
        if (pollSocket == mainServerSocket) {
            addNewSocket(serverTable, pollSocket);
            continue;
        } else if (pollSocket == STDIN_FILENO && getc(stdin) == 'e') break;

        /* Receive new packets */
        processClient(pollSocket, serverTable);

    }

    /* When finished, clean up*/
    freeTable(serverTable);
}
