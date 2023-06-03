
/* Simple UDP Client
 * Created by Bryce Melander in May 2023
 * using modified code from Dr. Hugh Smith
 * */

#include "rcopy.h"

void checkArgs(int argc, char *argv[], FILE **fp, runtimeArgs_t *usrArgs);

int setupUdpClientToServer(addrInfo_t *serverInfo, char *hostName, int hostPort);

addrInfo_t *setupTransfer(int socket, addrInfo_t *serverInfo, runtimeArgs_t *usrArgs, pollSet_t *pollSet);

void runClient(int socket, addrInfo_t *serverInfo, runtimeArgs_t *usrArgs, FILE *fp);

int main(int argc, char *argv[]) {

    FILE *fp;
    runtimeArgs_t usrArgs = {0};
    int socket;
    addrInfo_t *serverInfo = initAddrInfo();

    /* Verify user arguments */
    checkArgs(argc, argv, &fp, &usrArgs);

    /* Set up the UDP connection */
    socket = setupUdpClientToServer(serverInfo, usrArgs.host_name, usrArgs.host_port);

    /* Initialize the error rate library */
    sendErr_init(usrArgs.error_rate, DROP_ON, FLIP_ON, DEBUG_OFF, RSEED_ON);

    /* Start the transfer process */
    runClient(socket, serverInfo, &usrArgs, fp);

    return 0;
}

void checkArgs(int argc, char *argv[], FILE **fp, runtimeArgs_t *usrArgs) {

    /* check command line arguments  */
    if (argc != 8) {
        fprintf(
                stderr,
                "\nusage: ./rcopy "
                "from-filename "
                "to-filename "
                "window-size "
                "buffer-size "
                "error-percent "
                "remote-machine "
                "remote-port \n"
        );
        exit(EXIT_FAILURE);
    }

    usrArgs->from_filename = strdup(argv[ARGV_FROM_FILENAME]);

    /* Check if from-filename is shorter than 100 characters */
    if (strlen(usrArgs->from_filename) > 99) {
        fprintf(stderr, "\nError: from-filename must be no longer than 100 characters. \n");
        exit(EXIT_FAILURE);
    }

    *fp = fopen(usrArgs->from_filename, "r");

    /* Check if the given file exists */
    if (*fp == NULL) {
        fprintf(stderr, "\nError: file %s not found. \n", usrArgs->from_filename);
        exit(EXIT_FAILURE);
    }

    usrArgs->to_filename = strdup(argv[ARGV_TO_FILENAME]);

    /* Check if to-filename is shorter than 100 characters */
    if (strlen(usrArgs->to_filename) > 99) {
        fprintf(stderr, "\nError: to-filename must be no longer than 100 characters. \n");
        exit(EXIT_FAILURE);
    }

    usrArgs->error_rate = strtof(argv[ARGV_ERR_RATE], NULL);

    /* Check that error rate is in a valid range */
    if (usrArgs->error_rate >= 1.0 || usrArgs->error_rate < 0) {
        fprintf(stderr, "\nError: error-rate must be greater than or equal to 0, and less than 1. \n");
        exit(EXIT_FAILURE);
    }

    usrArgs->buffer_size = strtol(argv[ARGV_BUFFER_SIZE], NULL, 10);

    /* Check buffer size is within range */
    if (usrArgs->buffer_size == 0 || usrArgs->buffer_size > 1400) {
        fprintf(stderr, "\nError: buffer-size must be between 1 and 1400 (inclusive). \n");
        exit(EXIT_FAILURE);
    }

    usrArgs->window_size = strtol(argv[ARGV_WINDOW_SIZE], NULL, 10);

    /* Check window size is within its range */
    if (usrArgs->window_size > 0x40000000) {
        fprintf(stderr, "\nError: window-size must be less than 2^30. \n");
        exit(EXIT_FAILURE);
    }

    usrArgs->host_name = strdup(argv[ARGV_HOST_NAME]);
    usrArgs->host_port = (uint16_t) strtol(argv[ARGV_HOST_PORT], NULL, 10);

    /* Make sure a port was given */
    if (usrArgs->host_port < 1) {
        fprintf(stderr, "\nError: host-port must be greater than 0. \n");
        exit(EXIT_FAILURE);
    }

}

/*!
 * Opens a socket and fills in the serverAddress struct using the hostName and serverPort. \n\n
 * Written by Hugh Smith – April 2017
 * @param serverInfo A non-null sockaddr_in6 struct that has been initialized to 0
 * @param hostName
 * @param hostPort
 * @return A UDP socket connected to a UDP server.
 */
int setupUdpClientToServer(addrInfo_t *serverInfo, char *hostName, int hostPort) {

    int clientSocket;

    /* Open a new socket */
    clientSocket = socket(AF_INET6, SOCK_DGRAM, 0);
    if (clientSocket == -1) {
        fprintf(stderr, "Couldn't open socket!\n");
        perror("socket() call error");
        exit(EXIT_FAILURE);
    }

    /* Add the port to the address info struct */
    *((int *) serverInfo->addrInfo->sa_data) = htons(hostPort);

    /* Get the IP address */
    gethostbyname6(hostName, (struct sockaddr_in6 *) serverInfo->addrInfo);

    return clientSocket;
}

addrInfo_t *setupTransfer(int socket, addrInfo_t *serverInfo, runtimeArgs_t *usrArgs, pollSet_t *pollSet) {

    uint16_t count = 1, filenameLen = strlen(usrArgs->to_filename) + 1;
    uint16_t payloadLen = sizeof(uint16_t) + sizeof(uint32_t) + filenameLen;
    uint8_t payload[MAX_PAYLOAD_LEN] = {0};

    addrInfo_t *childInfo = initAddrInfo();
    packet_t *packet = initPacket();
    size_t pktLen;

    /** Establish a connection with the child process **/
    printf("Connecting to server subprocess...\n");

    /* Send the packet and wait for an Ack */
    while (1) {

        /* Check for timeout */
        if (count > 9) {
            printf("Child process could not reach client\n");
            return NULL;
        }

        /* Make a setup packet */
        pktLen = buildPacket(packet, 0, SETUP_PKT, NULL, 0);

        /* Send the setup packet */
        safeSendTo(socket, packet, pktLen, serverInfo);

        /* Check for socket activity */
        if (pollCall(pollSet, POLL_1_SEC) != POLL_TIMEOUT) {

            /* Get the server response message */
            pktLen = safeRecvFrom(socket, packet, pktLen + PDU_HEADER_LEN, childInfo);

            /* Verify the checksum and packet flag */
            if (in_cksum((unsigned short *) packet, (int) pktLen) == 0 && packet->flag == SETUP_ACK_PKT) break;

        } else {

            /* Increment timeout */
            count++;

        }
    }

    printf("Connection established!\n");

    /** Send the transfer info to the server **/
    printf("Sending transmission info...\n");

    /* Add buffer length */
    memcpy(&payload[HS_IDX_BUFF_LEN], &(usrArgs->buffer_size), sizeof(uint16_t));

    /* Add window size */
    memcpy(&payload[HS_IDX_WIND_LEN], &(usrArgs->window_size), sizeof(uint32_t));

    /* Add the to-filename */
    memcpy(&payload[HS_IDX_FILENAME], (usrArgs->to_filename), filenameLen);

    /* Fill the PDU */
    pktLen = buildPacket(packet, 0, INFO_PKT, payload, payloadLen);

    /* Wait for the server to respond */
    while (1) {

        /* If we get no response after 10 tries, assume the connection has been terminated */
        if (count > 10) {
            printf("Server has disconnected!\n");
            return NULL;
        } else count++;

        /* Send the PDU */
        safeSendTo(socket, (void *) packet, pktLen, childInfo);

        /* Check of rsocket activity */
        if (pollCall(pollSet, POLL_1_SEC) != POLL_TIMEOUT) {

            /* Receive packet */
            pktLen = safeRecvFrom(socket, (void *) packet, MAX_PDU_LEN, childInfo);

            /* Verify checksum */
            if (in_cksum((unsigned short *) packet, (int) pktLen) == 0) {

                /* Check packet flag */
                if (packet->flag == INFO_ACK_PKT) break;
                else if (packet->flag == INFO_ERR_PKT) {

                    printf("The server was unable to create the new file, please try again. \n");
                    return NULL;
                }
            }
        }
    }

    /* Clean up */
    free(packet);

    return childInfo;

}

void teardown(
        runtimeArgs_t *usrArgs, pollSet_t *pollSet, circularWindow_t *window,
        FILE *fp, addrInfo_t *addrInfo, packet_t *packet
) {

    freePollSet(pollSet);

    freeWindow(window);

    freeAddrInfo(addrInfo);

    free(usrArgs->to_filename);
    free(usrArgs->from_filename);
    free(usrArgs->host_name);

    free(packet);

    fclose(fp);
}

void runClient(int socket, addrInfo_t *serverInfo, runtimeArgs_t *usrArgs, FILE *fp) {

    uint32_t currentSeq = 0, recvSeq;
    uint16_t buffLen = usrArgs->buffer_size;
    uint8_t count = 0, eof = 0, dataBuff[MAX_PAYLOAD_LEN];
    size_t pktLen, readLen;
    ssize_t ret;

    /* Set up the packet and window */
    packet_t *packet = initPacket();
    circularWindow_t *packetWindow = createWindow(usrArgs->window_size);

    /* Set up the poll set */
    pollSet_t *pollSet = initPollSet();
    addToPollSet(pollSet, socket);

    /* Connect to the child process */
    addrInfo_t *childInfo = setupTransfer(socket, serverInfo, usrArgs, pollSet);

    /* Send the server the necessary transfer details */
    if (childInfo == NULL) {
        teardown(usrArgs, pollSet, packetWindow, fp, serverInfo, packet);
        return;
    } else {
        freeAddrInfo(serverInfo);
    }

    /* Transfer data */
    while (1) {

        /* Monitor connection */
        if (count > 9) {
            printf("Server has disconnected, shutting down...\n");
            teardown(usrArgs, pollSet, packetWindow, fp, childInfo, packet);
            return;
        }

        /* Fill the window */
        while (getWindowSpace(packetWindow)) {

            /* Read from the input file */
            readLen = (ssize_t) fread(dataBuff, sizeof(uint8_t), buffLen, fp);

            /* Check for EOF */
            if (feof(fp)) {

                /* Make a packet */
                pktLen = buildPacket(packet, currentSeq++, DATA_EOF_PKT, dataBuff, readLen);

                /* Add it to the window */
                addWindowPacket(packetWindow, packet, pktLen);

                /* Prevent any more packets being added */
                eof = 1;

                break;
            }

            /* Make a packet */
            pktLen = buildPacket(packet, currentSeq++, DATA_PKT, dataBuff, readLen);

            /* Add it to the window */
            addWindowPacket(packetWindow, packet, pktLen);
        }

        /* Check if we can send more packets */
        while (checkWindowOpen(packetWindow)) {

            /* Get the packet at current */
            pktLen = getWindowPacket(packetWindow, packet, WINDOW_CURRENT);

            /* Send the packet */
            safeSendTo(socket, (void *) packet, pktLen, childInfo);

            /* Don't send anything in the window after the EOF packet */
            if (packet->flag == DATA_EOF_PKT) break;
        }

        /* Receive data */
        while (1) {

            if (count > 9) break;

            /* Check if the window is open or closed */
            if (!eof && checkWindowOpen(packetWindow)) {

                /* Window open: non-blocking */
                ret = pollCall(pollSet, POLL_NON_BLOCK);

            } else {

                /* Window closed: blocking */
                ret = pollCall(pollSet, POLL_1_SEC);

            }

            /* Check if poll timed out */
            if (ret < 0) {

                /* If poll times out, resend the lowest packet */
                pktLen = getWindowPacket(packetWindow, packet, WINDOW_LOWER);

                /* Send packet */
                safeSendTo(socket, (void *) packet, pktLen, childInfo);

                /* Keep track of timeouts */
                count++;

                /* Try resending */
                if (eof && count < 9) continue;
                else break;
            }

            /* Receive the packet */
            pktLen = safeRecvFrom(socket, packet, buffLen + PDU_HEADER_LEN, childInfo);

            /* Verify checksum and packet type */
            if (in_cksum((unsigned short *) packet, (int) pktLen)) {

                /* If a server's reply becomes corrupt, we don't know what it wanted,
                 * so get the lowest packet in the window to get back on track */
                pktLen = getWindowPacket(packetWindow, packet, WINDOW_LOWER);

                /* Send packet */
                safeSendTo(socket, (void *) packet, pktLen, childInfo);

            } else if (packet->flag == DATA_REJ_PKT) {

                /* Get the rejected packet's sequence number */
                recvSeq = htonl(*((uint32_t *) packet->payload));

                /* Get the requested packet */
                pktLen = getWindowPacket(packetWindow, packet, (int) recvSeq);

                /* Send the packet */
                safeSendTo(socket, (void *) packet, pktLen, childInfo);

            } else if (packet->flag == DATA_ACK_PKT || packet->flag == TERM_CONN_PKT) {

                /* EOF waits until all RRs are received */
                if (eof == 1 && packet->flag != TERM_CONN_PKT) {

                    /* Re-send the RR if a timeout occurred after sending the EOF packet, as it was likely lost */
                    if (count > 0) {

                        recvSeq = htonl(*((uint32_t *) packet->payload)) + 1;

                        pktLen = getWindowPacket(packetWindow, packet, (int) recvSeq);
                        safeSendTo(socket, packet, pktLen, childInfo);

                    }

                    continue;
                }

                /* Get the RRs sequence number */
                recvSeq = htonl(*((uint32_t *) packet->payload));

                /* Move the window to the packet after the received packet sequence */
                moveWindow(packetWindow, recvSeq + 1);

                /* Reset count */
                count = 0;

                /* Once the window moves, we can send another packet */
                break;

            } /* Packets without the Ack or SRej flag are ignored */
        }

        /* Once a termination request has been received and all packets have been RRed, we're done sending data */
        if (packet->flag == TERM_CONN_PKT) break;
    }

    /* Make a termination ACK packet */
    pktLen = buildPacket(packet, currentSeq, TERM_ACK_PKT, NULL, 0);

    /* Send the final Ack and hope the server gets it */
    safeSendTo(socket, packet, pktLen, childInfo);

    printf("File transfer has been successfully transmitted!\n");

    /* Clean up! */
    teardown(usrArgs, pollSet, packetWindow, fp, childInfo, packet);
}
