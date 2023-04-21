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
            exit(-1);
        }
    }

    return bytesReceived;
}

int safeSend(int socketNum, uint8_t *buffer, int bufferLen, int flag) {
    int bytesSent;
    if ((bytesSent = (int) send(socketNum, buffer, bufferLen, flag)) < 0) {
        perror("recv call");
        exit(-1);
    }

    return bytesSent;
}


void *srealloc(void *ptr, size_t size) {
    void *returnValue = NULL;

    if ((returnValue = realloc(ptr, size)) == NULL) {
        printf("Error on realloc (tried for size: %d\n", (int) size);
        exit(-1);
    }

    return returnValue;
}

void *scalloc(size_t nmemb, size_t size) {
    void *returnValue = NULL;
    if ((returnValue = calloc(nmemb, size)) == NULL) {
        perror("calloc");
        exit(-1);
    }
    return returnValue;
}

