
#include "serverTable.h"

servTable_t *createTable(int size) {

    servTable_t *newTable = NULL;

    if (size < 0) return NULL;

    newTable->sockets = create_new_dict(size);
    newTable->handle = create_new_dict(size);

    return newTable;
}

int addClient(int socket, char *handle) {

    return 0;
}

int getTableSize(servTable_t *table) {
    return table->sockets->size;
}

/*char *getHandle(servTable_t *table, int socket) {

    return dict_search(table->handle, socket);
}*/

int getSocket(servTable_t *table, char *handle) {

    return dict_search(table->sockets, handle);
}

/*int removeClient(servTable_t *table, int socket) {

    char *handle = dict_remove(table->handle, socket);

    dict_remove(table->sockets, handle);

    return 0;
}*/
