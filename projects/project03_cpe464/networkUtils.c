
#include "networkUtils.h"

void *srealloc(void *ptr, size_t size) {

    void *ret = realloc(ptr, size);

    if (ret == NULL) {
        printf("Error using realloc with size %zu\n", size);
        perror("srealloc");
        exit(EXIT_FAILURE);
    }

    return ret;
}

void *scalloc(size_t count, size_t size) {

    void *ret = calloc(count, size);

    if (ret == NULL) {
        printf("Error using calloc with %zu items of size %zu\n", count, size);
        perror("scalloc");
        exit(EXIT_FAILURE);
    }

    return ret;
}

size_t safeRecvFrom(int socket, void *buf, size_t len, addrInfo_t *srcAddrInfo) {

    /* Check if we need to make a temp address struct */
    if (srcAddrInfo == NULL) {
        srcAddrInfo = initAddrInfo();
    }

//    printf("\n"
//           "recvfromErr args:   \n"
//           "Socket: %d          \n"
//           "Packet Flag: %d     \n"
//           "Len: %zu            \n"
//           "Port: %d            \n\n",
//           socket, ((packet_t *) buf)->flag, len,
//           ntohs(*((uint16_t *) srcAddrInfo->addrInfo->sa_data))
//    );

    /* Receive a packet */
    ssize_t ret = recvfrom(
            socket, buf, len, 0,
            srcAddrInfo->addrInfo,
            (socklen_t *) &srcAddrInfo->addrLen
    );

    /* Check for errors */
    if (ret < 0) {
        if (errno != EINTR) {
            printf("recvfromErr was interrupted!\n");
        } else {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }

    }

    return (size_t) ret;
}

size_t safeSendTo(int socket, void *buf, size_t len, addrInfo_t *dstAddrInfo) {

    if (len < 1) {
        printf("Cannot send packet of size zero!\n");
        return 0;
    }

    /* Send the packet */
    ssize_t ret = sendto(
            socket, buf, (int) len, 0, dstAddrInfo->addrInfo, dstAddrInfo->addrLen
    );

    /* Check for errors */
    if (ret < 0) {
        if (errno != EINTR) {
            printf("sendtoErr was interrupted!\n");
        } else {
            perror("sendto");
            exit(EXIT_FAILURE);
        }
    }

    return (size_t) ret;
}

addrInfo_t *initAddrInfo() {

    addrInfo_t *a = scalloc(1, sizeof(addrInfo_t));

    a->addrLen = sizeof(struct sockaddr_in6);

    a->addrInfo = scalloc(1, a->addrLen);

    a->addrInfo->sa_family = AF_INET6;

    return a;
}

packet_t *initPacket(void) {

    /* Allocate space for the header plus the payload, which is a Flexible Array Member */
    return (packet_t *) scalloc(1, sizeof(packet_t));
}

size_t
buildPacket(packet_t *packet, uint16_t payloadLen, uint32_t seqNum, uint8_t flag, uint8_t *data, size_t dataLen) {

    uint16_t checksum, pduLen = PDU_HEADER_LEN + dataLen;

    /* Check inputs */
    if (packet == NULL) {
        fprintf(stderr, "buildPacket() err! Null packet struct\n");
        exit(EXIT_FAILURE);
    } else if (dataLen > 1400) {
        fprintf(stderr, "buildPacket() err! Payload len > 1400\n");
        exit(EXIT_FAILURE);
    } else if (data == NULL && dataLen > 0) {
        fprintf(stderr, "buildPacket() err! Null payload with non-zero length\n");
        exit(EXIT_FAILURE);
    }

    /* Add the sequence in network order (4 bytes) */
    packet->seq_NO = htonl(seqNum);

    /* Set the checksum to 0 for now (2 bytes) */
    packet->checksum = 0;

    /* Add in the flag (1 byte) */
    packet->flag = flag;

    /* Clear old data */
    memset(packet->payload, 0, payloadLen);

    /* Add in the payload */
    memcpy(packet->payload, data, dataLen);

    /* Calculate the payload checksum */
    checksum = in_cksum((unsigned short *) packet, pduLen);

    /* Put the checksum into the PDU */
    packet->checksum = checksum;

    return pduLen;
}

void freeAddrInfo(addrInfo_t *addrInfo) {
    free(addrInfo->addrInfo);
    free(addrInfo);
}
