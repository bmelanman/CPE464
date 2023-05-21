/* Writen by Hugh Smith, Feb. 2021 */

#ifndef SAFEUTIL_H
#define SAFEUTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/socket.h>

#include "cpe464.h"

#define MAX_BUFF_LEN 80

#define PDU_SEQ 0
#define PDU_CHK 4
#define PDU_FLG 6
#define PDU_PLD 7

#define PDU_HEADER_LEN 7
#define MAX_PAYLOAD_LEN 1400

#define MAX_PDU_LEN (MAX_PAYLOAD_LEN + PDU_HEADER_LEN)

#define SETUP_PKT       1
#define SETUP_ACK_PKT   2

#define DATA_PKT        3
#define DATA_ACK_PKT    5
#define DATA_REJ_PKT    6
#define DATA_EOF_PKT    9

#define INFO_PKT        7
#define INFO_ACK_PKT    8

#define TERM_CONN_PKT   10
#define TERM_ACK_PKT    11

typedef struct pduPacketStruct {
    uint32_t seq_NO;
    uint16_t checksum;
    uint8_t flag;
    uint8_t payload[MAX_PAYLOAD_LEN];
} udpPacket_t;

struct sockaddr;

void *srealloc(void *ptr, size_t size);

void *scalloc(size_t nmemb, size_t size);

int safeRecvFrom(int socketNum, void *buf, int len, int flags, struct sockaddr *srcAddr, int *addrLen);

int safeSendTo(int socketNum, void *buf, int len, struct sockaddr *srcAddr, int addrLen);

int createPDU(udpPacket_t *pduPacket, uint32_t seqNum, uint8_t flag, uint8_t *payload, int payloadLen);

void printPDU(udpPacket_t *pduPacket, int pduLength);

#endif /* SAFEUTIL_H */
