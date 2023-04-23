
#ifndef PROJECT_2_SERVER_H
#define PROJECT_2_SERVER_H

#include <printf.h>
#include <unistd.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>

int checkArgs(int argc, char *argv[]);

void serverControl(int mainServerSocket);

#endif /* PROJECT_2_SERVER_H */
