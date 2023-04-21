/******************************************************************************
* myServer.c
* 
* Writen by Prof. Smith, updated Jan 2023
* Use at your own risk.  
*
*****************************************************************************/

#include <ctype.h>
#include "server.h"

int main(int argc, char *argv[]) {
    int mainServerSocket;   //socket descriptor for the server socket
    int portNumber;

    portNumber = checkArgs(argc, argv);

    // Set up poll
    setupPollSet();

    //create the server socket
    mainServerSocket = tcpServerSetup(portNumber);

    // Run the server
    serverControl(mainServerSocket);

    return 0;
}

void addNewSocket(int socket) {

    int clientSocket = tcpAccept(socket, 1);

    addToPollSet(clientSocket);
}

int processClient(int clientSocket) {

    uint8_t dataBuffer[MAXBUF];
    int messageLen;

    /* Now get the data from the client_socket */
    messageLen = recvPDU(clientSocket, dataBuffer, MAXBUF);

    /* Error checking */
    if (messageLen < 0) {
        perror("recv call");
        exit(-1);
    }

    /* If the message length is zero, that means the client has left */
    if (messageLen > 0) {
        printf("Message received, length: %d Data: %s\n", messageLen, dataBuffer);
    } else {
        printf("Connection closed by other side\n");
        removeFromPollSet(clientSocket);
        if (dataBuffer[0] == '%' && tolower(dataBuffer[1]) == 'e') {
            return 0;
        }
    }

    return 1;
}

void serverControl(int mainServerSocket) {

    int run_server = 1, sock;

    /* Add the main server socket to the list of sockets to poll */
    addToPollSet(mainServerSocket);

    while (run_server) {

        /* Call poll() non-blocking */
        sock = pollCall(0);

        /* If there's activity on the main server socket, add the new incoming socket */
        if (sock == mainServerSocket) addNewSocket(sock);

        /* Otherwise, look for new client packets */
        else if (sock != -1) run_server = processClient(sock);

    }

    /* When finished, clean up*/
    close(mainServerSocket);
}

int checkArgs(int argc, char *argv[]) {

    /* Checks args and returns port number */
    int portNumber = 0;

    /* Check for optional port number */
    if (argc > 2) {
        fprintf(stderr, "Usage %s [optional port number]\n", argv[0]);
        exit(-1);
    } else if (argc == 2) {
        portNumber = (int) strtol(argv[1], NULL, 10);
    }

    return portNumber;
}

