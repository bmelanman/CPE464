
#ifndef PROJECT_2_SERVER_H
#define PROJECT_2_SERVER_H

#include <printf.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>

#include "networkUtils.h"
#include "serverTable.h"

int checkArgs(int argc, char *argv[]);

int tcpServerSetup(int serverPort);

void serverControl(int mainServerSocket);

#endif /* PROJECT_2_SERVER_H */
