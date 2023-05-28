
#include "pollLib.h"

/* Easy to use poll library
 * Written by Hugh Smith - April 2022
 * Modified by Bryce Melander - May 2023
 *
 * Note: pollCall() always returns the lowest available file descriptor
 * which could cause higher file descriptors to never be processed
 * */

static void growPollSet(pollSet_t *pollSet, int newSetSize) {
    int i;

    /* Verify the need to increase the set size */
    if (newSetSize <= pollSet->pollSetSize) {
        fprintf(
                stderr,
                "Error - current poll set size: %d newSetSize is not greater: %d\n",
                pollSet->pollSetSize, newSetSize
        );
        exit(EXIT_FAILURE);
    }

    /* Re-allocate memory for the increased size */
    pollSet->pollFds = srealloc(pollSet->pollFds, newSetSize * sizeof(struct pollfd));

    /* Set the new elements to zero */
    for (i = pollSet->pollSetSize; i < newSetSize; i++) {
        pollSet->pollFds[i].fd = 0;
        pollSet->pollFds[i].events = 0;
    }

    /* Update the size */
    pollSet->pollSetSize = newSetSize;
}

pollSet_t *initPollSet(void) {

    pollSet_t *pollSet = scalloc(1, sizeof(pollSet_t));

    pollSet->pollSetSize = POLL_SET_SIZE;
    pollSet->pollFds = (struct pollfd *) scalloc(POLL_SET_SIZE, sizeof(struct pollfd));

    return pollSet;
}

void addToPollSet(pollSet_t *pollSet, int socketNumber) {

    if (socketNumber >= pollSet->pollSetSize) {
        /* Needs to increase off of the biggest socket number since
         * the file desc. may grow with files open or sockets
         * so socketNumber could be much bigger than currentPollSetSize
         * */
        growPollSet(pollSet, socketNumber + POLL_SET_SIZE);
    }

    /* Keep track of largest largest file descriptor */
    if (socketNumber + 1 >= pollSet->maxFd) {
        pollSet->maxFd = socketNumber + 1;
    }

    /* Add the info to the poll set */
    pollSet->pollFds[socketNumber].fd = socketNumber;
    pollSet->pollFds[socketNumber].events = POLLIN;
}

void removeFromPollSet(pollSet_t *pollSet, int socketNumber) {
    /* Clear out the old info */
    pollSet->pollFds[socketNumber].fd = -1;
    pollSet->pollFds[socketNumber].events = -1;
    /* Close the socket as well */
//    close(socketNumber);
}

int pollCall(pollSet_t *pollSet, int timeInMilliSeconds) {

    /* Calls poll() and returns the result after error checking
     *  - timeInMilliSeconds = -1 sets full blocking
     *  - timeInMilliSeconds = 0 sets non-blocking
     *  - Returns the socket number if one is ready for read
     *  - Returns -1 if timeout occurred
     * */

    int i, pollValue, returnValue = -1;

    pollValue = poll(pollSet->pollFds, pollSet->maxFd, timeInMilliSeconds);

    if (pollValue < 0) {
        if (errno == EINTR) {
            fprintf(stderr, "poll() was interrupted!\n");
        } else {
            perror("pollCall");
            exit(EXIT_FAILURE);
        }
    }

    /* Check to see if timeout occurred (poll returned 0) */
    if (pollValue > 0) {

        /* See which socket is ready */
        for (i = 0; i < pollSet->maxFd; i++) {
            /* Could just check for specific revents, but want to catch all of them
             * Otherwise, this could mask an error (eat the error condition) */

            /* if(pollSet->pollFds[i].revents & (POLLIN|POLLHUP|POLLNVAL)) {
                return i;
            } */

            if (pollSet->pollFds[i].revents > 0) {
                return i;
            }
        }
    }

    return returnValue;
}

void freePollSet(pollSet_t *pollSet) {

    int i, s;

    /* Ensure all sockets are closed */
    for (i = pollSet->maxFd; i >= 0; --i) {
        s = pollSet->pollFds[i].fd;
        if (s > 0) close(s);
    }

    /* Free previously allocated memory */
    free(pollSet->pollFds);
    free(pollSet);

}
