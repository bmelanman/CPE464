
#include "trace.h"

int main(int argc, char *argv[]) {

    char *buff = NULL, *debug = getenv("DEBUG");
    FILE *fp;

    pcap_header_t *pcap_header = malloc(PCAP_HEADER_LEN);
    packet_header_t *pkt_header = malloc(PKT_HEADER_LEN);

    unsigned int packet_num = 1, packet_len;
    uint32_t eth_start;

    /* Check for file input */
    if (argc < 2) {
        fprintf(stderr, "ERR: Please provide an input *.pcap file!\n");
        return 1;
    }

    /* Check for the correct file type */
    strtok_r(strdup(argv[1]), ".", &buff);
    if (strcmp(buff, "pcap") != 0) {
        fprintf(stderr, "ERR: The given file MUST be a *.pcap file!\n");
        return 1;
    }

    /* Make sure the file opens correctly */
    if ((fp = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "\nfopen(%s) failed, errno: %d\n", argv[1], errno);
        return 1;
    }

    /* Read the PCAP header into a struct */
    fread(pcap_header, PCAP_HEADER_LEN, 1, fp);

    /* Loop through each packet */
    while (1) {

        /* Read the Ethernet header into a struct */
        fread(pkt_header, PKT_HEADER_LEN, 1, fp);

        /* check for End Of File */
        if (feof(fp)) break;

        /* Display info */
        packet_len = pkt_header->org_len;
        printf("\nPacket number: %d  Frame Len: %d\n\n", packet_num, packet_len);

        /* Keep track of packet size via the file pointer position */
        eth_start = ftell(fp);

        /* Process the rest of the header */
        process_eth_h(fp);

        /* Make sure the file pointer is at the end of the
         * packet before moving on to the next packet */
        fseek(fp, eth_start + packet_len, SEEK_SET);

        /* Next packet! */
        packet_num++;

        /* Debug stuff */
        if (debug != NULL) {
            if (strcmp(debug, "1") == 0 && packet_num > 11) break;
        }
    }

    return 0;
}

/* mactostr: MAC Address to String
 * Converts a little-endian uint64_t MAC address to a human-readable string
 * (ex: 151138023047680 -> 00:02:2d:90:75:89)
 * */
char *mactostr(uint64_t mac_addr) {

    uint8_t i;
    uint64_t _mac_addr;
    char buff[3] = "", *dst = malloc(18);

    for (i = 0; i < 6; i++) {
        /* Get the current byte from the given address */
        _mac_addr = (mac_addr >> (i * 8)) & 0xff;

        /* Concatenate the byte onto the final address */
        /*snprintf(buff, 3, "%02llx", _mac_addr);*/
        snprintf(buff, 3, "%llx", _mac_addr);
        strncat(dst, buff, 3);

        /* Add colons where appropriate */
        if (i < 5) {
            strncat(dst, ":", 2);
        }
    }

    return dst;
}

/* iptostr: IP Address to String
 * Converts a little-endian uint64_t IP address to a human-readable string
 * (ex: 1711384768 -> 192.168.1.102)
 * */
char *iptostr(uint64_t ip_addr) {

    uint8_t i;
    uint64_t _ip_addr;
    char buff[4] = "", *dst = malloc(18);

    for (i = 0; i < 4; i++) {
        /* Grab the next byte from the given address */
        _ip_addr = (ip_addr >> (i * 8)) & 0xff;

        /* Concatenate the byte as a base-10 number onto the final address */
        snprintf(buff, 4, "%lld", _ip_addr);
        strncat(dst, buff, 3);

        /* Add dots where appropriate */
        if (i < 3) {
            strncat(dst, ".", 2);
        }
    }

    return dst;
}

/* get_type: Get type from protocol
 * Takes a given protocol type and returns the corresponding string
 * (ex: 1711384768 -> 192.168.1.102)
 * */
char *get_type(uint8_t protocol, uint16_t type) {

    char *unknown_type;
    
    switch (protocol) {
        case ETH_HEADER:
        case IPV4_HEADER:
        case ARP_HEADER:
            switch (type) {
                case 0x0100:
                    return "Request";
                case 0x0200:
                case 0x0004:
                    return "Reply";
                case 0x0608:
                    return "ARP";
                case 0x0008:
                    return "IP";
                case 0x0006:
                    return "TCP";
                case 0x0011:
                    return "UDP";
                case 0x0001:
                    return "ICMP";
                default:
                    return strdup("Unknown");
            }

        case TCP_HEADER:
            if (type == 0x5000) return " HTTP";
            unknown_type = malloc(10);
            snprintf(unknown_type, 8, ": %d", htons(type));
            return unknown_type;


        case ICMP_HEADER:
            switch (type) {
                case 0x0000:
                    return "Reply";
                case 0x0008:
                    return "Request";
                default:
                    unknown_type = malloc(10);
                    snprintf(unknown_type, 8, "%d", type);
                    return unknown_type;
            }

        default:
            fprintf(stderr, "function 'get_type()' received an unknown protocol: %d", protocol);
            exit(1);
    }
}

void process_eth_h(FILE *fp) {

    eth_header_t *eth_h = malloc(ETH_HEADER_LEN);

    /* Read eth header from file */
    fread(eth_h, ETH_HEADER_LEN, 1, fp);

    uint16_t type = eth_h->type;

    /* Print template */
    printf(
            "\tEthernet Header\n"
            "\t\tDest MAC: %s\n"
            "\t\tSource MAC: %s\n"
            "\t\tType: %s\n\n",
            mactostr(eth_h->dst_addr),
            mactostr(eth_h->src_addr),
            get_type(ETH_HEADER, type)
    );

    switch (type) {
        case 0x0608:
            process_arp_h(fp);
            break;
        case 0x0008:
            process_ip_h(fp);
            break;
        default:
            /*printf("default main loop case: 0x%02x\n", type);*/
            printf("default main loop case: 0x%x\n", type);
            break;
    }
}

void process_arp_h(FILE *fp) {

    arp_header_t *arp_h = malloc(ARP_HEADER_LEN);

    fread(arp_h, ARP_HEADER_LEN, 1, fp);

    printf(
            "\tARP header\n"
            "\t\tOpcode: %s\n"
            "\t\tSender MAC: %s\n"
            "\t\tSender IP: %s\n"
            "\t\tTarget MAC: %s\n"
            "\t\tTarget IP: %s\n\n",
            get_type(ARP_HEADER, arp_h->OPER),
            mactostr(arp_h->SHA), iptostr(arp_h->SPA),
            mactostr(arp_h->THA), iptostr(arp_h->TPA)
    );
}

void process_ip_h(FILE *fp) {

    char verify[10] = "Correct";

    ip_v4_header_t *ip_header = malloc(ARP_HEADER_LEN);
    fread(ip_header, IPV4_HEADER_LEN, 1, fp);

    uint16_t header_len = (ip_header->Ver_IHL & 0xf) * 4;
    uint8_t protocol = ip_header->protocol;

    /* TODO: Fix checksum */
    if (in_cksum((unsigned short *) ip_header, IPV4_HEADER_LEN)) {
        snprintf(verify, 10, "Incorrect");
    }

    printf(
            "\tIP Header\n"
            "\t\tHeader Len: %d (bytes)\n"
            /*"\t\tTOS: 0x%02x\n"*/
            "\t\tTOS: 0x%x\n"
            "\t\tTTL: %d\n"
            "\t\tIP PDU Len: %d (bytes)\n"
            "\t\tProtocol: %s\n"
            /*"\t\tChecksum: %s (0x%04x)\n"*/
            "\t\tChecksum: %s (0x%x)\n"
            "\t\tSender IP: %s\n"
            "\t\tDest IP: %s\n",
            header_len,
            ip_header->TOS,
            ip_header->TTL,
            htons(ip_header->len),
            get_type(IPV4_HEADER, protocol),
            verify, ip_header->cksum,
            iptostr(ip_header->src_addr),
            iptostr(ip_header->dst_addr)
    );

    if (header_len > IPV4_HEADER_LEN) {
        fseek(fp, (long) (header_len - IPV4_HEADER_LEN), SEEK_CUR);
    }

    switch (protocol) {
        case 0x1:
            printf("\n");
            process_icmp_h(fp);
            break;
        case 0x6:
            printf("\n");
            process_tcp_h(fp);
            break;
        case 0x11:
            printf("\n");
            process_udp_h(fp);
            break;
        default:
            break;
    }
}

void process_tcp_h(FILE *fp) {

    char ack[21] = "<not valid>";
    char verify[21] = "Correct";

    tcp_header_t *tcp_header = malloc(TCP_HEADER_LEN);

    fread(tcp_header, TCP_HEADER_LEN, 1, fp);

    /* TODO: Fix checksum */
    if (in_cksum((unsigned short *) tcp_header, TCP_HEADER_LEN)) {
        snprintf(verify, 20, "Incorrect");
    }

    uint16_t flags = htons(tcp_header->flags);
    uint32_t ack_num = htonl(tcp_header->ack);
    uint8_t ack_flag = flags & (0x01 << 4);

    if (ack_flag) {
        snprintf(ack, 20, "%u", ack_num);
    }

    printf(
            "\tTCP Header\n"
            "\t\tSource Port: %s\n"
            "\t\tDest Port: %s\n"
            "\t\tSequence Number: %u\n"
            "\t\tACK Number: %s\n"
            "\t\tACK Flag: %s\n"
            "\t\tSYN Flag: %s\n"
            "\t\tRST Flag: %s\n"
            "\t\tFIN Flag: %s\n"
            "\t\tWindow Size: %u\n"
            /*"\t\tChecksum: %s (0x%04x)\n",*/
            "\t\tChecksum: %s (0x%x)\n",
            get_type(TCP_HEADER, tcp_header->src_port),
            get_type(TCP_HEADER, tcp_header->dst_port),
            htonl(tcp_header->seq),
            ack,
            ack_flag ? "Yes" : "No",
            flags & (0x01 << 1) ? "Yes" : "No",
            flags & (0x01 << 2) ? "Yes" : "No",
            flags & (0x01 << 0) ? "Yes" : "No",
            htons(tcp_header->win_size),
            verify, htons(tcp_header->cksum)
    );
}

void process_icmp_h(FILE *fp) {

    icmp_header_t *icmp_header = malloc(ICMP_HEADER_LEN);

    fread(icmp_header, ICMP_HEADER_LEN, 1, fp);

    printf(
            "\tICMP Header\n"
            "\t\tType: %s\n",
            get_type(ICMP_HEADER, icmp_header->type)
    );

}

void process_udp_h(FILE *fp) {

    udp_header_t *udp_header = malloc(UDP_HEADER_LEN);

    fread(udp_header, UDP_HEADER_LEN, 1, fp);

    printf(
            "\tUDP Header\n"
            "\t\tSource Port: : %u\n"
            "\t\tDest Port: : %u\n",
            htons(udp_header->src_port),
            htons(udp_header->dst_port)
    );

}