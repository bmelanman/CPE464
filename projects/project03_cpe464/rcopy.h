
#ifndef LAB07_CPE464_RCOPY_H
#define LAB07_CPE464_RCOPY_H

#include "gethostbyname.h"
#include "networkUtils.h"
#include "pollLib.h"
#include "windowLib.h"

#define ARGV_FROM_FILENAME  1
#define ARGV_TO_FILENAME    2
#define ARGV_WINDOW_SIZE    3
#define ARGV_BUFFER_SIZE    4
#define ARGV_ERR_RATE       5
#define ARGV_HOST_NAME      6
#define ARGV_HOST_PORT      7

typedef struct argsStruct {
    char *from_filename;
    char *to_filename;
    uint32_t window_size;
    uint16_t buffer_size;
    float error_rate;
    char *host_name;
    uint16_t host_port;
} runtimeArgs_t;

#endif //LAB07_CPE464_RCOPY_H
