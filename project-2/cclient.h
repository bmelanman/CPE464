
#ifndef PROJECT_2_CCLIENT_H
#define PROJECT_2_CCLIENT_H

#include <ctype.h>
#include <printf.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#include "networkUtils.h"
#include "libPoll.h"

#define USR_CMD 1
#define CMD_SPC 2
#define CMD_DEST 3

void checkArgs(int argc, char *argv[]);

int clientInitTCP(char *handle, char *server_name, char *server_port);

void clientControl(int clientSocket, char *handle);

#endif /* PROJECT_2_CCLIENT_H */
