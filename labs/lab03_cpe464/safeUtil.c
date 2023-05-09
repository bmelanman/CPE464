/*
 * Writen by Hugh Smith, April 2020
 *
 * Put in system calls with error checking ands 'safe' to the name:
 *  - srealloc(): keep the function parameters same as system call
 * */

#include "safeUtil.h"

int safeRecv(int socketNum, uint8_t *buffer, int bufferLen, int flag) {

    int bytesReceived = (int) recv(socketNum, buffer, bufferLen, flag);

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

int safeSend(int socketNum, uint8_t *buffer, int bufferLen, int flag) {
    int bytesSent;

    bytesSent = (int) send(socketNum, buffer, bufferLen, flag);

    if (bytesSent < 0) {
        perror("recv call");
        exit(EXIT_FAILURE);
    }

    return bytesSent;
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

