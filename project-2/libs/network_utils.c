
#include "network_utils.h"

int sendPDU(int clientSocket, uint8_t *dataBuffer, int lengthOfData) {

    /* Calculate PDU length */
    int pdu_len = lengthOfData + PDU_HEADER_LEN;
    uint16_t pdu_len_no = htons(pdu_len), bytesSent;
    uint8_t tcp_pdu_buff[pdu_len];

    /* Fill the packet */
    memcpy(tcp_pdu_buff, &pdu_len_no, PDU_HEADER_LEN);
    memcpy(tcp_pdu_buff + PDU_HEADER_LEN, dataBuffer, lengthOfData);

    /* Send it off */
    bytesSent = (uint16_t) send(clientSocket, tcp_pdu_buff, pdu_len, 0);

    /* Error checking */
    if (bytesSent < 0) {
        perror("recv call");
        exit(EXIT_FAILURE);
    }

    return bytesSent;
}

int recvPDU(int socketNumber, uint8_t *dataBuffer, int bufferSize) {

    uint8_t msgLenBuff[2];
    uint16_t msgLen, bytesReceived;

    /* Get the header */
    bytesReceived = (uint16_t) recv(socketNumber, msgLenBuff, PDU_HEADER_LEN, MSG_WAITALL);

    /* Error checking */
    if (bytesReceived < 0) {
        if (errno == ECONNRESET) {
            bytesReceived = 0;
        } else {
            perror("recv call");
            exit(EXIT_FAILURE);
        }
    }

    /* Check if client has disconnected */
    if (bytesReceived == 0) {
        return 0;
    }

    /* Convert network order uint8 array to host order uint16 */
    msgLen = ntohs((msgLenBuff[1] << 8) | msgLenBuff[0]) - PDU_HEADER_LEN;

    /* Check message size */
    if (msgLen > bufferSize) {
        fprintf(stderr, "Given buffer is too small, packMessage size was %d bytes", msgLen);
        exit(EXIT_FAILURE);
    }

    /* Get the rest of the message */
    bytesReceived = (uint16_t) recv(socketNumber, dataBuffer, msgLen, MSG_WAITALL);

    /* Error checking */
    if (bytesReceived < 0) {
        if (errno == ECONNRESET) {
            bytesReceived = 0;
        } else {
            perror("recv call");
            exit(EXIT_FAILURE);
        }
    }

    return bytesReceived;
}

/* Hugh Smith April 2017 - Network code to support TCP/UDP client and server connections */
int tcpServerSetup(int serverPort) {

    /* This function sets the server socket. The function returns the server
     * socket number and prints the port number to the screen.
     * */

    /* Opens a server socket, binds that socket, prints out port, call listens
     * returns the mainServerSocket
     * */

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

int tcpAccept(int mainServerSocket, int debugFlag) {

    /* This function waits for a client to ask for services. It returns
     * the client socket number.
     * */

    struct sockaddr_in6 clientAddress;
    int client_socket, clientAddressSize = sizeof(clientAddress);

    client_socket = accept(mainServerSocket, (struct sockaddr *) &clientAddress, (socklen_t *) &clientAddressSize);

    if (client_socket < 0) {
        perror("accept call");
        exit(EXIT_FAILURE);
    }

    if (debugFlag) {
        printf(
                "Client accepted. Client IP: %s Client Port Number: %d\n",
                getIPAddressString6(clientAddress.sin6_addr.s6_addr),
                ntohs(clientAddress.sin6_port)
        );
    }

    return (client_socket);
}

static char *getIPAddressString46(unsigned char *ipAddress, int addressFamily);

static unsigned char *getIPAddress46(const char *hostName, struct sockaddr_storage *aSockaddr, int addressFamily);

char *ipAddressToString(struct sockaddr_in6 *ipAddressStruct) {

    static char ipString[INET6_ADDRSTRLEN];

    /* Put the IP address into a printable format */
    inet_ntop(AF_INET6, &ipAddressStruct->sin6_addr, ipString, sizeof(ipString));

    return ipString;
}

unsigned char *gethostbyname4(const char *hostName, struct sockaddr_in *aSockaddr) {

    /* Returns ipv4 address and fills in the aSockaddr with address (unless its NULL) */

    struct sockaddr_in aSockaddrTemp;
    struct sockaddr_in *aSockaddrPtr = aSockaddr;

    /* If user does not care about the struct make a temp one */
    if (aSockaddr == NULL) {
        aSockaddrPtr = &aSockaddrTemp;
    }

    return (getIPAddress46(hostName, (struct sockaddr_storage *) aSockaddrPtr, AF_INET));
}

unsigned char *gethostbyname6(const char *hostName, struct sockaddr_in6 *aSockaddr6) {

    /* Returns ipv6 address and fills in the aSockaddr6 with address (unless its NULL) */

    struct sockaddr_in6 aSockaddr6Temp;
    struct sockaddr_in6 *aSockaddr6Ptr = aSockaddr6;

    /* If user does not care about the struct make a temp one */
    if (aSockaddr6 == NULL) {
        aSockaddr6Ptr = &aSockaddr6Temp;
    }

    return (getIPAddress46(hostName, (struct sockaddr_storage *) aSockaddr6Ptr, AF_INET6));
}


char *getIPAddressString4(unsigned char *ipAddress) {
    return getIPAddressString46(ipAddress, AF_INET);
}

char *getIPAddressString6(unsigned char *ipAddress) {
    return getIPAddressString46(ipAddress, AF_INET6);
}


static char *getIPAddressString46(unsigned char *ipAddress, int addressFamily) {
    // makes it easy to print the IP address (v4 or v6)
    static char ipString[INET6_ADDRSTRLEN];

    if (ipAddress != NULL) {
        inet_ntop(addressFamily, ipAddress, ipString, sizeof(ipString));
    } else {
        strcpy(ipString, "(IP not found)");
    }

    return ipString;
}

static unsigned char *getIPAddress46(const char *hostName, struct sockaddr_storage *aSockaddr, int addressFamily) {
    // Puts host IPv6 (or mapped IPV4) into the aSockaddr6 struct and return pointer to 16 byte address (NULL on error)
    // Only pulls the first IP address from the list of possible addresses

    static unsigned char ipAddress[INET6_ADDRSTRLEN];

    uint8_t *returnValue = NULL;
    int addrError = 0;
    struct addrinfo hints;
    struct addrinfo *hostInfo = NULL;

    memset(&hints, 0, sizeof(hints));
    if (addressFamily == AF_INET) {
        hints.ai_family = AF_INET;
    } else {
        hints.ai_flags = AI_V4MAPPED | AI_ALL;
        hints.ai_family = AF_INET6;
    }

    if ((addrError = getaddrinfo(hostName, NULL, &hints, &hostInfo)) != 0) {
        fprintf(stderr, "Error getaddrinfo (host: %s): %s\n", hostName, gai_strerror(addrError));
        returnValue = NULL;
    } else {
        if (addressFamily == AF_INET) {
            memcpy(&((struct sockaddr_in *) aSockaddr)->sin_addr.s_addr,
                   &((struct sockaddr_in *) hostInfo->ai_addr)->sin_addr.s_addr, 4);
            memcpy(ipAddress, &((struct sockaddr_in *) aSockaddr)->sin_addr.s_addr, 4);
        } else {
            memcpy(((struct sockaddr_in6 *) aSockaddr)->sin6_addr.s6_addr,
                   &(((struct sockaddr_in6 *) hostInfo->ai_addr)->sin6_addr.s6_addr), 16);
            memcpy(ipAddress, &((struct sockaddr_in6 *) aSockaddr)->sin6_addr.s6_addr, 16);
        }

        returnValue = ipAddress;
        freeaddrinfo(hostInfo);
    }

    return returnValue;    // Either Null or IP address
}

void *srealloc(void *ptr, size_t size) {
    void *retVal = NULL;

    retVal = realloc(ptr, size);

    if (retVal == NULL) {
        printf("Error on realloc (tried for size: %d\n", (int) size);
        exit(EXIT_FAILURE);
    }

    return retVal;
}

void *scalloc(size_t nmemb, size_t size) {
    void *retVal = NULL;

    retVal = calloc(nmemb, size);

    if (retVal == NULL) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }

    return retVal;
}

//
// Written Hugh Smith, Updated: April 2022
// Use at your own risk.  Feel free to copy, just leave my name in it.
//

// Note this is not a robust implementation
// 1. It is about as un-thread safe as you can write code.  If you
//    are using pthreads do NOT use this code.
// 2. pollCall() always returns the lowest available file descriptor
//    which could cause higher file descriptors to never be processed
//
// This is for student projects, so I don't intend on improving this.

// Poll global variables
static struct pollfd *pollFileDescriptors;
static int maxFileDescriptor = 0;
static int currentPollSetSize = 0;

static void growPollSet(int newSetSize);

// Poll functions (setup, add, remove, call)
void setupPollSet(void) {
    currentPollSetSize = POLL_SET_SIZE;
    pollFileDescriptors = (struct pollfd *) scalloc(POLL_SET_SIZE, sizeof(struct pollfd));
}

void addToPollSet(int socketNumber) {

    if (socketNumber >= currentPollSetSize) {
        // needs to increase off of the biggest socket number since
        // the file desc. may grow with files open or sockets
        // so socketNumber could be much bigger than currentPollSetSize
        growPollSet(socketNumber + POLL_SET_SIZE);
    }

    if (socketNumber + 1 >= maxFileDescriptor) {
        maxFileDescriptor = socketNumber + 1;
    }

    pollFileDescriptors[socketNumber].fd = socketNumber;
    pollFileDescriptors[socketNumber].events = POLLIN;
}

void removeFromPollSet(int socketNumber) {
    pollFileDescriptors[socketNumber].fd = 0;
    pollFileDescriptors[socketNumber].events = 0;
}

int pollCall(int timeInMilliSeconds) {

    // TODO: Implement static FIFO queue that keeps track of active sockets?
    // returns the socket number if one is ready for read
    // returns -1 if timeout occurred
    // if timeInMilliSeconds == -1 blocks forever (until a socket ready)
    // (this -1 is a feature of poll)
    // If timeInMilliSeconds == 0 it will return immediately after looking at the poll set

    int i = 0;
    int returnValue = -1;
    int pollValue = 0;

    if ((pollValue = poll(pollFileDescriptors, maxFileDescriptor, timeInMilliSeconds)) < 0) {
        perror("pollCall");
        exit(EXIT_FAILURE);
    }

    // check to see if timeout occurred (poll returned 0)
    if (pollValue > 0) {
        // see which socket is ready
        for (i = 0; i < maxFileDescriptor; i++) {
            //if(pollFileDescriptors[i].revents & (POLLIN|POLLHUP|POLLNVAL))
            //Could just check for specific revents, but want to catch all of them
            //Otherwise, this could mask an error (eat the error condition)
            if (pollFileDescriptors[i].revents > 0) {
                //printf("for socket %d poll revents: %d\n", i, pollFileDescriptors[i].revents);
                returnValue = i;
                break;
            }
        }
    }

    // Ready socket # or -1 if timeout/none
    return returnValue;
}

static void growPollSet(int newSetSize) {
    int i = 0;

    // just check to see if someone screwed up
    if (newSetSize <= currentPollSetSize) {
        printf("Error - current poll set size: %d newSetSize is not greater: %d\n",
               currentPollSetSize, newSetSize);
        exit(EXIT_FAILURE);
    }

    //printf("Increasing poll set from: %d to %d\n", currentPollSetSize, newSetSize);
    pollFileDescriptors = srealloc(pollFileDescriptors, newSetSize * sizeof(struct pollfd));

    // zero out the new poll set elements
    for (i = currentPollSetSize; i < newSetSize; i++) {
        pollFileDescriptors[i].fd = 0;
        pollFileDescriptors[i].events = 0;
    }

    currentPollSetSize = newSetSize;
}
