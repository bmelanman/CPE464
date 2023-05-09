
#ifndef LAB_3_SERVER_H
#define LAB_3_SERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <unistd.h>
#include <string.h>

#include "networks.h"
#include "libPDU.h"
#include "pollLib.h"

#define MAXBUF 1024
#define DEBUG_FLAG 1

int checkArgs(int argc, char *argv[]);

void serverControl(int mainServerSocket);

#endif /* LAB_3_SERVER_H */
