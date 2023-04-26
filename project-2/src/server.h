
#ifndef PROJECT_2_SERVER_H
#define PROJECT_2_SERVER_H

#include <printf.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/tcp.h>

#include "networkUtils.h"
#include "serverTable.h"

#define DISCONN_CLIENT 1
#define ADD_CLIENT 2

int checkArgs(int argc, char *argv[]);

int tcpServerSetup(int serverPort);

void serverControl(int mainServerSocket);

#endif /* PROJECT_2_SERVER_H */
