
#include "server.h"

static volatile int shutdownServer = 0;

void intHandler(void) {
    printf("\n\nShutting down server...\n");
    shutdownServer = 1;
}

int main(int argc, char *argv[]) {

    int mainServerSocket, portNumber;

    /* Check inout arguments */
    portNumber = checkArgs(argc, argv);

    /* Catch for ^C */
    signal(SIGINT, (void (*)(int)) intHandler);

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

    if (setsockopt(mainServerSocket, SOL_SOCKET, SO_REUSEADDR, &(int) {1}, sizeof(int)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
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

void checkSocketDisconnected(int bytesSent, serverTable_t *serverTable, char *clientHandle, int clientSocket) {

    if (bytesSent < 1) {

        /* Client disconnected from server */
        fprintf(stderr, "\nClient unexpectedly disconnected! \n");

        if (clientHandle != NULL) {

            removeClient(serverTable, clientHandle);

        } else if (clientSocket > -1) {

            removeClientSocket(serverTable, clientSocket);

        } else {

            /* Function input error handling */
            fprintf(stderr, "\ncheckSocketDisconnected() input error! Terminating...\n");
            exit(EXIT_FAILURE);
        }
    }
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

        /* Check if the client disconnected */
        checkSocketDisconnected(bytesSent, serverTable, clientHandle, -1);

    } else {

        /* Decline the connection */
        sendPDU(clientSocket, NULL, 0, CONN_ERR_PKT);
        removeFromPollSet(serverTable->pollSet, clientSocket);

    }
}

void sendToAll(serverTable_t *serverTable, int clientSocket, int flag, uint8_t sendBuff[], int pduLen) {

    /* Variables are initialized for broadcast packets */
    int i, bytesSent, sock = clientSocket;
    uint8_t handleLen, sendLen = pduLen;
    char *handle = NULL;

    /* Send each handle as in a packet 12 */
    for (i = 0; i < serverTable->arrCap; ++i) {

        /* Get the next handle */
        handle = serverTable->handleArr[i];

        /* Check for valid handles */
        if (handle == NULL || handle[0] == '\0') continue;

        /* Check if we need to switch to handshake packets */
        if (flag == HDL_LIST_PKT) {

            /* Pack a packet 12, skipping the first 3 bytes */
            handleLen = (int) strnlen(handle, MAX_HANDLE_LEN);
            memcpy(sendBuff, &handleLen, 1);
            memcpy(&sendBuff[1], handle, ++handleLen);

            /* Set the packet length */
            sendLen = handleLen + 1;

        } else {

            /* Send the broadcast to all valid sockets */
            sock = i;

            /* Don't send broadcast back to self */
            if (sock == clientSocket) continue;

        }

        bytesSent = sendPDU(sock, &sendBuff[1], sendLen, flag);
        checkSocketDisconnected(bytesSent, serverTable, NULL, clientSocket);
    }
}

void routeBroadcast(int clientSocket, serverTable_t *serverTable, uint8_t dataBuff[], int pduLen) {
    sendToAll(serverTable, clientSocket, BROADCAST_PKT, dataBuff, pduLen);
}

void routeMessage(int clientSocket, serverTable_t *serverTable, uint8_t dataBuff[], int pduLen) {

    char dstHandle[MAX_HDL];
    int dstSocket;

    /* Skip chat header, src handle info, and number of destinations */
    uint8_t handleLen = dataBuff[1];
    uint8_t offset = handleLen + 3;

    /* Get the destination handle */
    handleLen = dataBuff[offset++] + 1;
    snprintf(dstHandle, handleLen, "%s", &dataBuff[offset]);

    dstSocket = getClient(serverTable, dstHandle);

    /* Check if the destination client exists in our serverTable */
    if (dstSocket == -1) {

        /* Decrement offset so that it will point to the dst handle length */
        offset--;

        /* Send an error packet containing the length of the handle + the handle itself */
        sendPDU(clientSocket, &dataBuff[offset], handleLen + 1, DST_ERR_PKT);

        return;
    }

    /* Send the message packet to the destination client with the chat header removed (it will be added again by sendPDU) */
    sendPDU(dstSocket, &dataBuff[1], pduLen, MESSAGE_PKT);

}

void routeMulticast(int clientSocket, serverTable_t *serverTable, uint8_t dataBuff[], int pduLen) {

    /* Skip the src handle */
    uint8_t handleLen = dataBuff[1];
    uint8_t offset = handleLen + 2;
    uint8_t numDests = dataBuff[offset++];
    int i, dstSocket;
    char handle[MAX_HDL];

    for (i = 0; i < numDests; ++i) {

        handleLen = dataBuff[offset++];
        memcpy(handle, &dataBuff[offset], handleLen);
        handle[handleLen] = '\0';

        /* Get the next destination handle */
        dstSocket = getClient(serverTable, handle);

        /* Check for invalid handles */
        if (dstSocket == -1) {

            /* Send an error packet containing the length of the handle + the handle itself */
            sendPDU(clientSocket, &dataBuff[offset - 1], handleLen + 1, DST_ERR_PKT);

        } else {

            /* Send the packet with the header removed to the destination client */
            sendPDU(dstSocket, &dataBuff[1], pduLen, MULTICAST_PKT);

        }

        offset += handleLen;

    }

}

void sendClose(serverTable_t *serverTable, int clientSocket) {

    int bytesSent = sendPDU(clientSocket, NULL, 0, ACK_EXIT_PKT);

    if (bytesSent > 0) printf("Close request acknowledged, sent %d bytes\n", bytesSent);

    removeClientSocket(serverTable, clientSocket);
}

void sendList(serverTable_t *serverTable, int clientSocket) {

    uint8_t bytesSent, sendBuff[MAX_USR];
    uint32_t numClients = htonl(serverTable->size);

    /* Send a packet 11 containing the number of clients in the serverTable */
    memcpy(sendBuff, &numClients, 4);
    bytesSent = sendPDU(clientSocket, sendBuff, 4, ACK_LIST_PKT);

    /* Verify connection status */
    checkSocketDisconnected(bytesSent, serverTable, NULL, clientSocket);

    /* Send packet 12s for all client handles */
    sendToAll(serverTable, clientSocket, HDL_LIST_PKT, sendBuff, 0);

    /* Finish with a packet 13 */
    bytesSent = sendPDU(clientSocket, NULL, 0, FIN_LIST_PKT);
    checkSocketDisconnected(bytesSent, serverTable, NULL, clientSocket);

}

void processClient(int clientSocket, serverTable_t *serverTable) {

    uint8_t recvBuffer[MAX_USR];
    int messageLen;

    /* Now get the data from the client_socket */
    messageLen = recvPDU(clientSocket, recvBuffer, MAX_USR);

    /* If the message length is -1, the client has disconnected */
    if (messageLen == 0) {
        removeClientSocket(serverTable, clientSocket);
    }

    switch (recvBuffer[PDU_FLAG]) {
        case 1:     /* New client handshake */
            processNewClient(clientSocket, recvBuffer, serverTable);
            break;
        case 4:     /* Broadcast packet */
            routeBroadcast(clientSocket, serverTable, recvBuffer, messageLen);
            break;
        case 5:     /* Message packet */
            routeMessage(clientSocket, serverTable, recvBuffer, messageLen);
            break;
        case 6:     /* Multicast packet */
            routeMulticast(clientSocket, serverTable, recvBuffer, messageLen);
            break;
        case 8:     /* Close request */
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
        pollSocket = callTablePoll(serverTable, POLL_WAIT_FOREVER);

        /* No new packets (useful for debugging when 'poll() interrupted' errors can occur more frequently) */
        if (pollSocket == -1) continue;

        /* Check for new sockets */
        if (pollSocket == mainServerSocket) {
            addNewSocket(serverTable, pollSocket);
            continue;
        } else if (pollSocket == STDIN_FILENO) {
            if (fgetc(stdin) == 'e') break; // debugging
            else continue;
        }

        /* Receive new packets */
        processClient(pollSocket, serverTable);

    }

    /* When finished, clean up*/
    freeTable(serverTable);
}
