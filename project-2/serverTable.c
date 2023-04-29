
#include "serverTable.h"

int hash(char *handle) {

    /* modified djb2 hashing algorithm, from: http://www.cse.yorku.ca/~oz/hash.html */

    int hash = 0;

    if (handle == NULL) return NOT_FOUND;

    while (*handle != '\0') {
        hash = ((hash << 5) + hash) + *handle++;
    }

    return abs(hash);
}

serverTable_t *newServerTable(int size) {

    /* Init a new dictionary */
    serverTable_t *newTable;

    if (size < 1) return NULL;

    /* Allocate the requested amount of memory */
    newTable = malloc(sizeof(serverTable_t));
    if (newTable == NULL) { MEM_ERR("serverTable.c") }

    newTable->nodes = malloc(sizeof(tableNode_t) * size);
    if (newTable->nodes == NULL) { MEM_ERR("serverTable.c") }

    /* Make a new pollSet */
    newTable->pollSet = newPollSet();

    /* Initialize the handle array */
    newTable->handleArr = NULL;
    newTable->arrCap = 0;

    /* Set the size and capacity */
    newTable->tableCap = size;
    newTable->size = 0;

    return newTable;
}

tableNode_t *newTableNode(char *handle, int socket, tableNode_t *next) {

    /* Check input */
    if (handle == NULL || handle[0] == '\0') return NULL;

    /* Init a new node */
    tableNode_t *node = malloc(sizeof(tableNode_t));
    if (node == NULL) { MEM_ERR("dictionary.c") }

    /* Check for NULL input */
    if (handle == NULL) node->handle = strndup("", 1);
    else node->handle = strndup(handle, MAX_HANDLE_LEN);

    node->socket = socket;
    node->next = next;

    return node;
}

serverTable_t *increaseTableCap(serverTable_t *oldTable) {

    int i, idx, newSize, oldSize = oldTable->tableCap;

    /* Check input */
    if (oldTable == NULL) return NULL;

    /* Grab the old nodes */
    tableNode_t **oldNodeArr = oldTable->nodes, *oldNode, *newNode;

    /* Calculate the new size */
    newSize = 2 * oldSize + 1;

    /* Create a new serverTable */
    serverTable_t *newTable = newServerTable(newSize);

    /* We grabbed the first oldVal already, start at idx 1 */
    for (i = 0; i < oldSize; i++) {

        /* Get the next oldNode in the array */
        oldNode = oldNodeArr[i];

        /* Add each oldNode to the newTable as a newNode  */
        while (oldNode != NULL && oldNode->handle != NULL) {

            /* Hash the handle */
            idx = hash(oldNode->handle) % newTable->tableCap;

            /* Don't traverse the list because we already know that all nodes are unique */

            /* Add a new node to the dictionary at the beginning of the linked list */
            newNode = newTableNode(oldNode->handle, oldNode->socket, newTable->nodes[idx]);
            newTable->nodes[idx] = newNode;
            newTable->size++;

            /* Continue to the next node */
            oldNode = oldNode->next;
        }
    }

    /* Copy over everything else as-is */
    newTable->handleArr = oldTable->handleArr;
    newTable->arrCap = oldTable->arrCap;
    newTable->pollSet = oldTable->pollSet;

    return newTable;
}

int addClient(serverTable_t *serverTable, int socket, char *handle) {

    int idx;
    tableNode_t *node;

    /* Check input */
    if (serverTable == NULL || handle == NULL || handle[0] == '\0') return 1;

    /* Make sure the serverTable has enough space for another node */
    if (serverTable->size >= serverTable->tableCap) {
        memcpy(serverTable, increaseTableCap(serverTable), sizeof(*serverTable));
        if (serverTable == NULL) { MEM_ERR("dictionary.c") }
    }

    /* Hash the handle */
    idx = hash(handle) % serverTable->tableCap;
    node = serverTable->nodes[idx];

    /* Traverse the linked list */
    while (node != NULL) {

        if (strcmp(handle, node->handle) == 0) return 1;

        /* Go to the next node in the linked list */
        node = node->next;
    }

    if (node == NULL) {
        /* Add the new node at the top of the list */
        serverTable->nodes[idx] = newTableNode(handle, socket, NULL);
    } else {
        /* Add a new node to the end of the linked list */
        node->next = newTableNode(handle, socket, NULL);
    }

    /* Increase the table size */
    serverTable->size++;

    /* Make sure the handle array is big enough for the new socket */
    if (socket >= serverTable->arrCap) {
        serverTable->arrCap = socket + 1;
        serverTable->handleArr = srealloc(serverTable->handleArr, (sizeof(char *)) * serverTable->arrCap);
    }

    /* Copy the new handle into the handle array, then increment its size */
    serverTable->handleArr[socket] = strndup(handle, MAX_HANDLE_LEN);

    return 0;
}

int getClient(serverTable_t *serverTable, char *handle) {

    /* Check input */
    if (serverTable == NULL || handle == NULL) return NOT_FOUND;

    /* Hash the given handle */
    int hashed_handle = hash(handle) % serverTable->tableCap;

    /* Get the head node */
    tableNode_t *node = serverTable->nodes[hashed_handle];

    /* If there is no head node, return not found */
    if (node == NULL) {
        return NOT_FOUND;
    }

    /* Check each node for the given handle */
    while (node != NULL) {

        /* If the handle is found, return its value */
        if (strcmp(handle, node->handle) == 0) return node->socket;

        /* Go to the next node */
        node = node->next;
    }

    /* Return -1 if the handle is not found in the dictionary */
    return NOT_FOUND;
}

int removeClient(serverTable_t *table, char *handle) {

    int removedSocket, hashedHandle;

    /* Check input */
    if (table == NULL || handle == NULL) return NOT_FOUND;

    /* Hash the given handle */
    hashedHandle = hash(handle) % table->tableCap;

    /* Get the head node */
    tableNode_t *node = table->nodes[hashedHandle];

    /* Make sure the head node exists */
    if (node == NULL) return NOT_FOUND;

    /* If the head node contains the handle, remove it */
    if (strcmp(handle, node->handle) == 0) {

        /* Copy the socket to be removed */
        removedSocket = node->socket;

        /* Remove the node from the linked list */
        if (node->next != NULL) {
            table->nodes[hashedHandle] = node->next;
        } else {
            table->nodes[hashedHandle] = NULL;
        }

        /* Clean up */
        free(node->handle);
        free(node);

        /* Remove the node from the pollSet */
        removeFromPollSet(table->pollSet, removedSocket);

        /* Remove the handle from the handle array */
        table->handleArr[removedSocket][0] = '\0';

        /* Update the table size */
        table->size--;

        return removedSocket;
    }

    /* Check each node for the given handle */
    while (node->next != NULL) {

        /* If the handle is found, return its value */
        if (strcmp(handle, node->next->handle) == 0) {

            /* Copy the socket to be removed */
            removedSocket = node->next->socket;
            /* Remove the node from the linked list */
            node->next = node->next->next;

            /* Clean up */
            free(node->handle);
            free(node);

            /* Remove the node from the pollSet */
            removeFromPollSet(table->pollSet, removedSocket);

            /* Remove the handle from the handle array */
            table->handleArr[removedSocket][0] = '\0';

            /* Update the table size */
            table->size--;

            return removedSocket;
        }

        /* Go to the next node */
        node = node->next;
    }

    /* Return -1 if the node is not found in the dictionary */
    return NOT_FOUND;

}

int removeClientSocket(serverTable_t *table, int clientSocket) {

    /* Get the client's handle from the handle array */
    char *clientHandle = table->handleArr[clientSocket];

    /* Now that we have the handle, we can remove the client */
    removeClient(table, clientHandle);

    /* Remove the socket from the pollSet */
    removeFromPollSet(table->pollSet, clientSocket);

    return 0;
}

void addToPollTable(serverTable_t *serverTable, int socket) {
    addToPollSet(serverTable->pollSet, socket);
}

int callTablePoll(serverTable_t *serverTable, int timeout) {
    return pollCall(serverTable->pollSet, timeout);
}

void freeTable(serverTable_t *serverTable) {

    int i;

    if (serverTable == NULL) return;

    freePollSet(serverTable->pollSet);

    for (i = 0; i < serverTable->tableCap; i++) {

        tableNode_t *node = serverTable->nodes[i];

        while (node != NULL) {

            /* Grab the node pointer first */
            tableNode_t *temp = node;

            /* Go to the next node before free()ing the current one */
            node = node->next;

            /* free() the current node */
            if (temp->handle != NULL) free(temp->handle);
            if (temp != NULL) free(temp);
        }
    }

    /* free() everything else */
    free(serverTable->nodes);
    free(serverTable);
}
