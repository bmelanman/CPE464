
#ifndef PROJECT_2_SERVERTABLE_H
#define PROJECT_2_SERVERTABLE_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "libPoll.h"

/* ENOMEM definition taken from sys/errno.h */
#define ENOMEM 12
#define MEM_ERR(STR) printf("%s malloc err", STR); errno = ENOMEM; exit(errno);

#define MAX_HANDLE_LEN 101
#define NOT_FOUND (-1)

typedef struct tableNode tableNode_t;

struct tableNode {
    char *handle;
    int socket;
    tableNode_t *next;
};

typedef struct serverTable {
    tableNode_t **nodes;    /* An array of linked lists containing handle/socket pairs      */
    int tableCap;           /* The maximum capacity of the table                            */
    int size;               /* Current number of elements in the table                      */
    pollSet_t *pollSet;     /* See libPoll.c                                                */
    char **handleArr;       /* An array of handles with the socket used as the handle idx   */
    int arrCap;             /* The maximum capacity of the handle array                     */
} serverTable_t;

serverTable_t *newServerTable(int size);

int addClient(serverTable_t *serverTable, int socket, char *handle);

int getClient(serverTable_t *serverTable, char *handle);

int removeClient(serverTable_t *table, char *handle);

int removeClientSocket(serverTable_t *table, int clientSocket);

void addToPollTable(serverTable_t *serverTable, int socket);

int callTablePoll(serverTable_t *serverTable, int timeout);

void freeTable(serverTable_t *serverTable);

#endif /* PROJECT_2_SERVERTABLE_H */
