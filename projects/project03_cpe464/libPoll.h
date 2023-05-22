
#ifndef PROJECT_2_LIBPOLL_H
#define PROJECT_2_LIBPOLL_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <poll.h>

#include "networkUtils.h"

#define POLL_SET_SIZE 10
#define POLL_TIMEOUT (-1)

#define POLL_BLOCK (-1)
#define POLL_1_SEC 1000
#define POLL_10_SEC 10000

typedef struct pollSetStruct {
    struct pollfd *pollFds;
    int maxFd;
    int pollSetSize;
} pollSet_t;

pollSet_t * newPollSet(void);

void addToPollSet(pollSet_t *pollSet, int socketNumber);

void removeFromPollSet(pollSet_t *pollSet, int socketNumber);

int pollCall(pollSet_t *pollSet, int timeInMilliSeconds);

void freePollSet(pollSet_t *pollSet);

#endif /* PROJECT_2_LIBPOLL_H */
