
#ifndef PROJECT_2_NETWORKUTILS_H
#define PROJECT_2_NETWORKUTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>

#define MAX_USR 1400
#define MAX_HDL 100
#define MAX_MSG 200
/* Max input / Max message len = 1400 / 200 = 7 */
#define MAX_PKTS 7

#define LISTEN_BACKLOG  10

#define PDU_MSG_LEN     2
#define PDU_HEADER_LEN  3

#define PDU_FLAG        0
#define PDU_SRC_LEN_IDX 1

#define CONN_PKT        1
#define CONN_ACK_PKT    2
#define CONN_ERR_PKT    3
#define BROADCAST_PKT   4
#define MESSAGE_PKT     5
#define MULTICAST_PKT   6
#define DST_ERR_PKT     7
#define REQ_EXIT_PKT    8
#define ACK_EXIT_PKT    9
#define REQ_LIST_PKT    10
#define ACK_LIST_PKT    11
#define HDL_LIST_PKT    12
#define FIN_LIST_PKT    13

void *srealloc(void *ptr, size_t size);

void *scalloc(size_t nmemb, size_t size);

int sendPDU(int clientSocket, uint8_t dataBuffer[], int lengthOfData, uint8_t pduFlag);

int recvPDU(int socketNumber, uint8_t dataBuffer[], int bufferSize);

#endif /* PROJECT_2_NETWORKUTILS_H */
