
#include "dictionary.h"

int hash(char *key) {

    /* modified djb2 hashing algorithm, from: http://www.cse.yorku.ca/~oz/hash.html */

    int hash = 0;

    while (*key != '\0') {
        hash = ((hash << 5) + hash) + *key++;
    }

    return abs(hash);
}

dict *create_new_dict(int size) {

    /* Init a new dictionary */
    dict *new_dict;

    if (size < 0)
        return NULL;

    /* Allocate the requested amount of memory */
    new_dict = malloc(sizeof(dict));
    if (new_dict == NULL) { MEM_ERR("dictionary.c", 0) }

    new_dict->Nodes = malloc(sizeof(dict_node_t) * size);
    if (new_dict->Nodes == NULL) { MEM_ERR("dictionary.c", 1) }

    /* Set the size and capacity */
    new_dict->cap = size;
    new_dict->size = 0;

    return new_dict;
}

dict_node_t *create_dict_node(char *key, int value) {

    /* Init a new node */
    dict_node_t *node = malloc(sizeof(dict_node_t));
    if (node == NULL) { MEM_ERR("dictionary.c", 2) }

    node->key = strndup(key, strlen(key));
    node->val = value;
    node->next = NULL;

    return node;
}

dict *add_dict_capacity(dict *old_dict) {

    int i;
    dict_node_t *node = NULL;

    int old_size = old_dict->cap;
    dict_node_t **old_vals = old_dict->Nodes;

    int new_size = 2 * old_size + 1;

    dict *new_dict = create_new_dict(new_size);

    for (i = 0; i < old_size; i++) {

        node = old_vals[i];

        while (node != NULL && node->key != NULL) {
            dict_insert(new_dict, node->key, node->val);
            node = node->next;
        }
    }

    return new_dict;
}

int dict_insert(dict *dict, char *key, int value) {

    int idx;
    dict_node_t *node;

    /* Check for empty keys */
    if (key == NULL || key[0] == '\0') return -1;

    /* Make sure the dict has enough space for another node */
    if (dict->size >= dict->cap) {
        memcpy(dict, add_dict_capacity(dict), sizeof(*dict));
        if (dict == NULL) { MEM_ERR("dictionary.c", 2) }
    }

    /* Hash the key */
    idx = hash(key) % dict->cap;
    node = dict->Nodes[idx];

    /* Traverse the linked list */
    while (node != NULL) {

        if (strcmp(key, node->key) == 0) return -1;

        /* Go to the next node in the linked list */
        node = node->next;
    }

    /* Add a new node to the dictionary at the top of the linked list */
    node = create_dict_node(key, value);
    node->next = dict->Nodes[idx];
    dict->Nodes[idx] = node;
    dict->size++;

    return 0;
}

int dict_remove(dict *dict, char *key) {

    /* Hash the given key */
    int hashed_key = hash(key) % dict->cap, removed_val;

    /* Get the head node */
    dict_node_t *node = dict->Nodes[hashed_key];

    /* Make sure the head node exists */
    if (node == NULL) return -1;

    /* If the head node contains the key, remove it */
    if (strcmp(key, node->key) == 0) {
        removed_val = node->val;
        dict->Nodes[hashed_key] = node->next;
        free(node);
        return removed_val;
    }

    /* Check each node for the given key */
    while (node->next != NULL) {

        /* If the key is found, return its value */
        if (strcmp(key, node->next->key) == 0) {
            removed_val = node->next->val;
            node->next = node->next->next;
            free(node);
            return removed_val;
        }

        /* Go to the next node */
        node = node->next;
    }

    /* Return -1 if the node is not found in the dictionary */
    return -1;

}

int dict_search(dict *dict, char *key) {

    /* Hash the given key */
    int hashed_key = hash(key) % dict->cap;

    /* Get the head node */
    dict_node_t *node = dict->Nodes[hashed_key];

    /* If there is no head node, return not found */
    if (node == NULL) {
        return -1;
    }

    /* Check each node for the given key */
    while (node != NULL) {

        /* If the key is found, return its value */
        if (strcmp(key, node->key) == 0) return node->val;

        /* Go to the next node */
        node = node->next;
    }

    /* Return -1 if the node is not found in the dictionary */
    return -1;
}

void free_hash(dict *dict) {

    int i;

    for (i = 0; i < dict->cap; i++) {

        dict_node_t *node = dict->Nodes[i];

        while (node != NULL) {
            dict_node_t *temp = node;
            node = node->next;

            free(temp->key);
            free(temp);
        }
    }

    free(dict->Nodes);
    free(dict);
}
