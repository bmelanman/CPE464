
#ifndef PROJECT_1_TRACE_H
#define PROJECT_1_TRACE_H

/* Need because M1 Mac */
#if __APPLE__
#include <net/ethernet.h>
#else
#include <netinet/ether.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pcap/pcap.h>
#include <netinet/in.h>
#include <errno.h>

#include "libs/checksum.h"

#define ETH_HEADER  1
#define ARP_HEADER  2
#define IPV4_HEADER 3
#define TCP_HEADER  4
#define ICMP_HEADER 5

#define PCAP_HEADER_LEN   sizeof(pcap_header_t   )
#define ETH_HEADER_LEN    sizeof(eth_header_t    )
#define ARP_HEADER_LEN    sizeof(arp_header_t    )
#define IPV4_HEADER_LEN   sizeof(ip_v4_header_t  )
#define TCP_HEADER_LEN    sizeof(ip_v4_header_t  )
#define ICMP_HEADER_LEN   sizeof(icmp_header_t   )
#define UDP_HEADER_LEN    sizeof(udp_header_t    )
#define PSEUDO_HEADER_LEN sizeof(pseudo_header_t )
 
/* 24 bytes (192 bits) */
typedef struct __attribute__((packed)) pcap_header {    /* Offset */
    uint32_t magic;                                     /*      0 */
    uint16_t version_major;                             /*     32 */
    uint16_t version_minor;                             /*     48 */
    int32_t thiszone;                                   /*     64 */
    uint32_t sigfigs;                                   /*     96 */
    uint32_t snaplen;                                   /*    128 */
    uint32_t linktype;                                  /*    160 */
} pcap_header_t;

/* 16 bytes (128 bits) */
typedef struct __attribute__((packed)) packet_header {  /* Offset */
    uint32_t t_sec;                                     /*      0 */
    uint32_t t_usec;                                    /*     32 */
    uint32_t cap_len;                                   /*     64 */
    uint32_t org_len;                                   /*     96 */
} packet_header_t;

/* 14 bytes (112 bits) */
typedef struct __attribute__((packed)) eth_header {     /* Offset */
    uint8_t dst_addr[6];                                /*      0 */
    uint8_t src_addr[6];                                /*     48 */
    uint16_t type;                                      /*     96 */
} eth_header_t;

/* 28 bytes (224 bits) */
typedef struct __attribute__((packed)) arp_header {     /* Offset */
    uint16_t H_TYPE;                                    /*      0 */
    uint16_t P_TYPE;                                    /*     16 */
    uint8_t H_LEN;                                      /*     32 */
    uint8_t P_LEN;                                      /*     40 */
    uint16_t OPER;                                      /*     48 */
    uint8_t SHA[6];                                     /*     64 */
    uint32_t SPA;                                       /*    112 */
    uint8_t THA[6];                                     /*    144 */
    uint32_t TPA;                                       /*    192 */
} arp_header_t;

/* 20 bytes (160 bits) */
typedef struct __attribute__((packed)) ip_v4_header {   /* Offset */
    uint8_t Ver_IHL;                                    /*    0/4 */
    uint8_t TOS;                                        /*      8 */
    uint16_t len;                                       /*     16 */
    uint16_t ID;                                        /*     32 */
    uint16_t Flags_Offset;                              /*     48 */
    uint8_t TTL;                                        /*     64 */
    uint8_t protocol;                                   /*     72 */
    uint16_t cksum;                                     /*     80 */
    uint32_t src_addr;                                  /*     96 */
    uint32_t dst_addr;                                  /*    128 */
} ip_v4_header_t;

/* 20 bytes (160 bits) */
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

/* 12 bytes (96 bits) */
typedef struct __attribute__((packed)) pseudo_header {  /* Offset */
    uint32_t src_addr;                                  /*      0 */
    uint32_t dst_addr;                                  /*     32 */
    uint16_t type;                                      /*     64 */
    uint16_t tcp_len;                                   /*     80 */
} pseudo_header_t;

/* 4 bytes (32 bits) */
typedef struct __attribute__((packed)) icmp_header {    /* Offset */
    uint8_t type;                                       /*      0 */
    uint8_t code;                                       /*      8 */
    uint16_t cksum;                                     /*     16 */
    uint32_t data;                                      /*     32 */
} icmp_header_t;

/* 8 bytes (64 bits) */
typedef struct __attribute__((packed)) udp_header {     /* Offset */
    uint16_t src_port;                                  /*      0 */
    uint16_t dst_port;                                  /*     16 */
    uint16_t udp_len;                                   /*     32 */
    uint16_t cksum;                                     /*     48 */
} udp_header_t;

char *iptostr(uint32_t ip_addr);

char *get_type(uint8_t protocol, uint16_t type);

void process_eth_h(uint8_t *packet_data);

void process_arp_h(uint8_t *packet_data);

void process_ip_h(uint8_t *packet_data);

void process_tcp_h(uint8_t *packet_data, pseudo_header_t *pseudo_header);

void process_icmp_h(uint8_t *packet_data);

void process_udp_h(uint8_t *packet_data);

#endif /* PROJECT_1_TRACE_H */
