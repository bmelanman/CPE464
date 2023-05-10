
#ifndef LAB07_CPE464_UDPPDU_H
#define LAB07_CPE464_UDPPDU_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>
#include <memory.h>

#include "cpe464.h"

#define MAX_PAYLOAD_LEN 1400

#define PDU_SEQ 0
#define PDU_CHK 4
#define PDU_FLG 6
#define PDU_PLD 7

#define PDU_HEADER_LEN 7
#define MAX_PDU_LEN MAX_PAYLOAD_LEN + PDU_HEADER_LEN

int createPDU(uint8_t pduBuff[], uint32_t seqNum, uint8_t flag, uint8_t payload[], int payloadLen);

void printPDU(uint8_t pduBuff[], int pduLength);

#endif //LAB07_CPE464_UDPPDU_H
