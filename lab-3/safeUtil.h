// 
// Writen by Hugh Smith, Jan. 2023
//
// Put in system calls with error checking.

#ifndef __SAFEUTIL_H__
#define __SAFEUTIL_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#include "networks.h"

int safeRecv(int socketNum, uint8_t * buffer, int bufferLen, int flag);
int safeSend(int socketNum, uint8_t * buffer, int bufferLen, int flag);

void * srealloc(void *ptr, size_t size);
void * scalloc(size_t nmemb, size_t size);

#endif