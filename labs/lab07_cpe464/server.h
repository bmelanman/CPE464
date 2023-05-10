
#ifndef LAB07_CPE464_SERVER_H
#define LAB07_CPE464_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "gethostbyname.h"
#include "networks.h"
#include "safeUtil.h"

#define MAXBUF 80

void processClient(int socketNum);

void checkArgs(int argc, char *argv[], int *port, float *errRate);

#endif //LAB07_CPE464_SERVER_H
