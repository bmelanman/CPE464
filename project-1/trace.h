
#ifndef PROJECT_1_TRACE_H
#define PROJECT_1_TRACE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap/pcap.h>
#include <errno.h>
#include "libs/checksum.h"

#define PCAP_HEADER_LEN sizeof(pcap_header_t    )
#define PKT_HEADER_LEN  sizeof(packet_header_t  )
#define ETH_HEADER_LEN  sizeof(eth_header_t     )
#define ARP_HEADER_LEN  sizeof(arp_header_t     )
#define IPV4_HEADER_LEN sizeof(ip_v4_header_t   )
#define TCP_HEADER_LEN  sizeof(ip_v4_header_t   )
#define ICMP_HEADER_LEN sizeof(icmp_header_t    )
#define UDP_HEADER_LEN  sizeof(udp_header_t     )

#define PKT_HEADER  0
#define ETH_HEADER  1
#define ARP_HEADER  2
#define IPV4_HEADER 3
#define TCP_HEADER  4
#define ICMP_HEADER 5
#define UDP_HEADER  6

// 24 bytes (192 bits)
typedef struct __attribute__((packed)) pcap_header {    /* Offset */
    uint32_t magic;                                     /*      0 */
    uint16_t version_major;                             /*     32 */
    uint16_t version_minor;                             /*     48 */
    int32_t thiszone;                                   /*     64 */
    uint32_t sigfigs;                                   /*     96 */
    uint32_t snaplen;                                   /*    128 */
    uint32_t linktype;                                  /*    160 */
} pcap_header_t;

// 16 bytes (128 bits)
typedef struct __attribute__((packed)) packet_header {  /* Offset */
    uint32_t t_sec;                                     /*      0 */
    uint32_t t_usec;                                    /*     32 */
    uint32_t cap_len;                                   /*     64 */
    uint32_t org_len;                                   /*     96 */
} packet_header_t;

// 14 bytes (112 bits)
typedef struct __attribute__((packed)) eth_header {     /* Offset */
    uint64_t dst_addr: 48;                              /*      0 */
    uint64_t src_addr: 48;                              /*     48 */
    uint16_t type;                                      /*     96 */
} eth_header_t;

// 28 bytes (224 bits)
typedef struct __attribute__((packed)) arp_header {     /* Offset */
    uint16_t H_TYPE;                                    /*      0 */
    uint16_t P_TYPE;                                    /*     16 */
    uint8_t H_LEN;                                      /*     32 */
    uint8_t P_LEN;                                      /*     40 */
    uint16_t OPER;                                      /*     48 */
    uint64_t SHA: 48;                                   /*     64 */
    uint32_t SPA;                                       /*    112 */
    uint64_t THA: 48;                                   /*    144 */
    uint32_t TPA;                                       /*    192 */
} arp_header_t;

// 20 bytes (160 bits)
typedef struct __attribute__((packed)) ip_v4_header {   /* Offset */
    uint8_t Ver_IHL;                                    /*    0/4 */
    uint8_t DSCP_ECN;                                   /*   8/14 */
    uint16_t len;                                       /*     16 */
    uint16_t ID;                                        /*     32 */
    uint16_t Flags_Offset;                              /*     48 */
    uint8_t TTL;                                        /*     64 */
    uint8_t protocol;                                   /*     72 */
    uint16_t cksum;                                     /*     80 */
    uint32_t src_addr;                                  /*     96 */
    uint32_t dst_addr;                                  /*    128 */
} ip_v4_header_t;

// 20 bytes (160 bits)
typedef struct __attribute__((packed)) tcp_header {     /* Offset */
    uint16_t src_port;                                  /*      0 */
    uint16_t dst_port;                                  /*     16 */
    uint32_t seq;                                       /*     32 */
    uint32_t ack;                                       /*     64 */
    uint16_t flags;                                     /*     96 */
    uint16_t win_size;                                  /*    112 */
    uint16_t cksum;                                     /*    128 */
    uint16_t urg_ptr;                                   /*    144 */
} tcp_header_t;

// 4 bytes (32 bits)
typedef struct __attribute__((packed)) icmp_header {    /* Offset */
    uint8_t type: 4;                                    /*      0 */
    uint8_t code: 4;                                    /*      4 */
    uint8_t cksum;                                      /*      8 */
    uint16_t data;                                      /*     16 */
} icmp_header_t;

// 8 bytes (64 bits)
typedef struct __attribute__((packed)) udp_header {     /* Offset */
    uint16_t src_port;                                  /*      0 */
    uint16_t dst_port;                                  /*     16 */
    uint16_t udp_len;                                   /*     32 */
    uint16_t cksum;                                     /*     48 */
} udp_header_t;

char *mactostr(uint64_t mac_addr);

char *iptostr(uint64_t ip_addr);

char *get_type(uint8_t protocol, uint16_t code);

void get_eth_header(FILE *fp);

void get_arp_header(FILE *fp);

void get_ip_header(FILE *fp);

void get_tcp_header(FILE *fp, ip_v4_header_t *ip_header);

void get_icmp_header(FILE *fp);

void get_udp_header(FILE *fp);

#endif //PROJECT_1_TRACE_H
