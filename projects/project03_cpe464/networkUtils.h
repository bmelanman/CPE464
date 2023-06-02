/* Writen by Hugh Smith, Feb. 2021 */

#ifndef SAFEUTIL_H
#define SAFEUTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include <memory.h>
#include <errno.h>
#include <netinet/in.h>

#include "cpe464.h"

#define PDU_HEADER_LEN 7
#define MAX_PAYLOAD_LEN 1400
#define NO_PAYLOAD 0

#define MAX_PDU_LEN (MAX_PAYLOAD_LEN + PDU_HEADER_LEN)

#define CHILD_PROCESS 0

#define HS_IDX_BUFF_LEN     0
#define HS_IDX_WIND_LEN     2
#define HS_IDX_FILENAME     6

#define SETUP_PKT       1
#define SETUP_ACK_PKT   2

#define DATA_PKT        3
#define DATA_ACK_PKT    5
#define DATA_REJ_PKT    6
#define DATA_EOF_PKT    10

#define INFO_PKT        7
#define INFO_ACK_PKT    8
#define INFO_ERR_PKT    9

#define TERM_CONN_PKT   11
#define TERM_ACK_PKT    12

typedef struct addrInfo_s {
    int addrLen;
    struct sockaddr *addrInfo;
} addrInfo_t;

typedef struct packetStruct_s {
    uint32_t seq_NO;
    __attribute__((unused)) uint16_t checksum;
    uint8_t flag;
    uint8_t payload[MAX_PAYLOAD_LEN];
} packet_t;

struct sockaddr;

void *srealloc(void *ptr, size_t size);

void *scalloc(size_t count, size_t size);

size_t safeRecvFrom(int socket, void *buf, size_t len, addrInfo_t *srcAddrInfo);

size_t safeSendTo(int socket, void *buf, size_t len, addrInfo_t *dstAddrInfo);

addrInfo_t *initAddrInfo(void);

packet_t *initPacket(void);

size_t
buildPacket(packet_t *packet, uint32_t seqNum, uint8_t flag, uint8_t *data, size_t dataLen);

void freeAddrInfo(addrInfo_t *addrInfo);

#endif /* SAFEUTIL_H */
