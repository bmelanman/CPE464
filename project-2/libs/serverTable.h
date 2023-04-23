
#ifndef PROJECT_2_SERVERTABLE_H
#define PROJECT_2_SERVERTABLE_H

#include "dictionary.h"

typedef struct serverTable {
    dict *sockets;
    dict *handle;
} servTable_t;

#endif /* PROJECT_2_SERVERTABLE_H */
