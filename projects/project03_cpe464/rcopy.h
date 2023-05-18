
#ifndef LAB07_CPE464_RCOPY_H
#define LAB07_CPE464_RCOPY_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"

#define MAXBUF 80

void talkToServer(int socketNum, struct sockaddr_in6 *server);

int readFromStdin(char *buffer, int buffLen);

void checkArgs(int argc, char *argv[], int *port, float *errRate);

#endif //LAB07_CPE464_RCOPY_H
