
#ifndef PROJECT_2_CCLIENT_H
#define PROJECT_2_CCLIENT_H

#include <ctype.h>
#include <unistd.h>

#include "../libs/network_utils.h"

int sendToServer(int socket, uint8_t *sendBuf, int sendLen);

int readFromStdin(uint8_t *buffer);

void checkArgs(int argc, char *argv[]);
void clientControl(int clientSocket, char *handle);
void processPacket(int socket);
int clientInitTCP(char *handle, char *server_name, char *server_port);

int processUsrInput(int socket, char *handle);

void packMessage(int socket, char *clientHandle, char *dstHandle, char *usrMsg);

void broadcast(void);

void multicast(void);

void list(void);

#endif /* PROJECT_2_CCLIENT_H */
