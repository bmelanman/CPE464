/******************************************************************************
* myClient.c
*
* Writen by Prof. Smith, updated Jan 2023
* Use at your own risk.  
*
*****************************************************************************/

#include "networks.h"
#include "libPDU.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1

int sendToServer(int socketNum, uint8_t *sendBuf, int sendLen);

int readFromStdin(uint8_t *buffer);

void checkArgs(int argc, char *argv[]);

int main(int argc, char *argv[]) {

    int socketNum, sendLen, sendActual;
    uint8_t run_client = 1, sendBuf[MAXBUF] = {0};

    /* Check input arguments */
    checkArgs(argc, argv);

    /* Set up the TCP Client socket with the server name and port  */
    socketNum = tcpClientSetup(argv[1], argv[2], DEBUG_FLAG);

    /* Run the client */
    while (run_client != 0) {

        /* Get user input */
        sendLen = readFromStdin(sendBuf);

        /* Sent the message to the server */
        sendActual = sendToServer(socketNum, sendBuf, sendLen) - PDU_HEADER_LEN;

        /* If the data did not send, the server has disconnected */
        if (sendActual != sendLen) break;

        /* Check for exit code */
        if (sendBuf[0] == '%' && tolower(sendBuf[1]) == 'e') break;
    }

    /* Clean up */
    close(socketNum);

    return 0;
}

int sendToServer(int socketNum, uint8_t *sendBuf, int sendLen) {

    int sent;

    printf(
            "string len: %d (including null)\n"
            "message: %s\n",
            sendLen, sendBuf
    );

    /* Send the data as a packet with a header */
    sent = sendPDU(socketNum, sendBuf, sendLen);

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
    if (argc != 3) {
        printf("usage: %s host-name port-number \n", argv[0]);
        exit(EXIT_FAILURE);
    }
}
