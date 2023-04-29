
#ifndef PROJECT_2_LIBPOLL_H
#define PROJECT_2_LIBPOLL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#define POLL_SET_SIZE 10

typedef struct pollSetStruct {
    struct pollfd *pollFds;
    int maxFd;
    int pollSetSize;
} pollSet_t;

void *srealloc(void *ptr, size_t size);

pollSet_t * newPollSet(void);

void addToPollSet(pollSet_t *pollSet, int socketNumber);

void removeFromPollSet(pollSet_t *pollSet, int socketNumber);

int pollCall(pollSet_t *pollSet, int timeInMilliSeconds);

void freePollSet(pollSet_t *pollSet);

#endif /* PROJECT_2_LIBPOLL_H */
