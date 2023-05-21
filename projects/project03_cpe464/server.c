
/* Simple UDP Server
 * Created by Bryce Melander in May 2023
 * using modified code from Dr. Hugh Smith
 *  */

#include "server.h"
#include "libPoll.h"

static volatile int shutdownServer = 0;

void intHandler(void) {
    printf("\n\nShutting down server...\n");
    shutdownServer = 1;
}

void checkArgs(int argc, char *argv[], float *errRate, int *port);

int udpServerSetup(int serverPort);

void runServerController(int serverSocket);

int main(int argc, char *argv[]) {
    int socket, port;
    float errorRate;

    /* Catch for ^C */
    signal(SIGINT, (void (*)(int)) intHandler);

    /* Verify user input */
    checkArgs(argc, argv, &errorRate, &port);

    /* Set up UDP */
    socket = udpServerSetup(port);

    /* Initialize sendErr() */
    sendErr_init(errorRate, DROP_ON, FLIP_ON, DEBUG_ON, RSEED_OFF);

    /* Connect to a client */
    runServerController(socket);

    /* Clean up */
    close(socket);

    return 0;
}

/*!
 * Creates a UDP socket on the server side and binds to that socket. \n\n
 * Written by Hugh Smith – April 2017
 * @param serverPort
 * @return The server's socket
 */
int udpServerSetup(int serverPort) {
    struct sockaddr_in6 serverAddress;
    int socketNum;
    int serverAddrLen = 0;

    // create the socket
    if ((socketNum = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
        perror("socket() call error");
        exit(-1);
    }

    // set up the socket
    memset(&serverAddress, 0, sizeof(struct sockaddr_in6));
    serverAddress.sin6_family = AF_INET6;            // internet (IPv6 or IPv4) family
    serverAddress.sin6_addr = in6addr_any;        // use any local IP address
    serverAddress.sin6_port = htons(serverPort);   // if 0 = os picks

    // bind the name (address) to a port
    if (bind(socketNum, (struct sockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        perror("bind() call error");
        exit(-1);
    }

    /* Get the port number */
    serverAddrLen = sizeof(serverAddress);
    getsockname(socketNum, (struct sockaddr *) &serverAddress, (socklen_t *) &serverAddrLen);
    printf("Server using Port #: %d\n", ntohs(serverAddress.sin6_port));

    return socketNum;

}

void recvSetup(int socket) {

}

void runServer(int socket) {

    uint16_t pduLen, seq, transferComplete = 0;

    struct sockaddr_in6 client;
    int clientAddrLen = sizeof(client);
    udpPacket_t dataPDU = {0};

    while (!transferComplete) {

        pduLen = safeRecvFrom(socket, &dataPDU, MAX_PDU_LEN, 0, (struct sockaddr *) &client, &clientAddrLen);

        printf("IP: %s Port: %d\n", ipAddressToString(&client), ntohs(client.sin6_port));

        printf("Received PDU from client\n\n");

        printPDU(&dataPDU, pduLen);

        seq = ntohl(dataPDU.seq_NO);

        /* Send an ACK after receiving */
        pduLen = createPDU(&dataPDU, 0, DATA_ACK_PKT, (uint8_t *) &seq, sizeof(uint16_t));

        safeSendTo(socket, dataBuff, pduLen, (struct sockaddr *) &client, clientAddrLen);

        printf("\n");
    }
}

void runServerController(int serverSocket) {

    int pollSocket;
    pollSet_t *pollSet = newPollSet();

    addToPollSet(pollSet, serverSocket);

    while (!shutdownServer) {

        /* Wait for a client to connect */
        pollSocket = pollCall(pollSet, POLL_BLOCK);

        if (pollSocket == serverSocket) {
            /* Fork children here in the future */
            runServer(serverSocket);
        }

    }

}

void checkArgs(int argc, char *argv[], float *errRate, int *port) {
    // Checks args and returns port number

    /* User can only give 1 or 2 arguments */
    if (argc != 2 && argc != 3) {
        fprintf(stderr, "\nusage: ./server error-rate [port-number]\n");
        exit(EXIT_FAILURE);
    }

    /* Get the error rate */
    *errRate = strtof(argv[1], NULL);

    /* Check that error rate is in a valid range */
    if (*errRate >= 1.0 || *errRate < 0) {
        fprintf(stderr, "\nError: error-rate must be greater than or equal to 0, and less than 1\n");
        exit(EXIT_FAILURE);
    }

    /* Check if a port is provided */
    if (argc == 3) {
        *port = (int) strtol(argv[ARGV_PORT_NUM], NULL, 10);
    } else {
        *port = 0;
    }

}


