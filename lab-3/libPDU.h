
#ifndef LAB_3_LIBPDU_H
#define LAB_3_LIBPDU_H

#include "safeUtil.h"
#include "networks.h"

#define PDU_HEADER_LEN 2

int sendPDU(int clientSocket, uint8_t *dataBuffer, int lengthOfData);

int recvPDU(int socketNumber, uint8_t *dataBuffer, int bufferSize);

#endif /* LAB_3_LIBPDU_H */
