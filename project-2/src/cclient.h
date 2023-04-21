
#ifndef PROJECT_2_CCLIENT_H
#define PROJECT_2_CCLIENT_H

#include <printf.h>
#include <stdlib.h>
#include <string.h>

#define BUFF_SIZE 1400

int network_init(char *server_name, char *handle, char *server_port);

void network_run(int client_socket);

void message(void);

void broadcast(void);

void multicast(void);

void list(void);

#endif /* PROJECT_2_CCLIENT_H */
