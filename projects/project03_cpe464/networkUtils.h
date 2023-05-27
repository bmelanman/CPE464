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

#define MAX_PDU_LEN (MAX_PAYLOAD_LEN + PDU_HEADER_LEN)

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

typedef struct pduPacket_s {
    uint32_t seq_NO;
    __attribute__((unused)) uint16_t checksum;
    uint8_t flag;
    uint8_t payload[];
} udpPacket_t;

struct sockaddr;

void *srealloc(void *ptr, size_t size);

void *scalloc(size_t nmemb, size_t size);

int safeRecvFrom(int socketNum, void *buf, int len, addrInfo_t *srcAddr);

int safeSendTo(int socketNum, void *buf, int len, addrInfo_t *dstInfo);

addrInfo_t *initAddrInfo(void);

udpPacket_t *initPacket(uint16_t bufferLen);

int
createPDU(udpPacket_t *pduPacket, uint16_t bufferLen, uint32_t seqNum, uint8_t flag, uint8_t *payload, int payloadLen);

#endif /* SAFEUTIL_H */
