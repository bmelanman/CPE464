
/*
 * Credit to bryanlimy on GitHub, their dynamic hash_map helped a lot in certain areas of this code.
 * I started by using notes from 203, but a lot of this ended up looking very similar to bryanlimy's implementation.
 * Their repo can be found here: https://github.com/bryanlimy/dynamic-hash-table
 */

#ifndef DICTIONARY_H
#define DICTIONARY_H

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* ENOMEM definition taken from sys/errno.h */
#define ENOMEM 12
#define MEM_ERR(s, i) printf("%s malloc err at idx %d", s, i); exit(ENOMEM);

typedef struct dict_Node_struct dict_Node;
typedef struct dict_struct dict;

struct dict_Node_struct {
    char *key;
    int val;
    dict_Node *next;
};

struct dict_struct {
    dict_Node **Nodes;
    int cap;
    int size;
};

dict *create_new_dict(int size);

int dict_insert(dict *dict, char *key, int value);

int dict_remove(dict *dict, char *key);

int dict_search(dict *dict, char *key);

void free_hash(dict *dict);

#endif /* DICTIONARY_H */
