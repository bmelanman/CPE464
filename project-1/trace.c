
#include "trace.h"

/* Connection Oriented (3 Phases):
 *      1. Setup - Path between users is set
 *      2. Use - Path is used for communication
 *      3. Teardown - The path is no longer in use, so it is destroyed
 *      *  Used by protocols like TCP
 * Connectionless (1 Phase):
 *      1. Use - The data is addressed and sent, much like mailing a letter
 *      *  Used by protocols like UDP
 */

int main(int argc, char *argv[]) {

    if (argc < 2) {
        return 0;
    }

    pcap_header_t *pcap_header = malloc(PCAP_HEADER_LEN);
    packet_header_t *pkt_header = malloc(PKT_HEADER_LEN);

    unsigned int packet_num = 1, packet_len;
    uint32_t eth_start;

    FILE *fp = fopen(argv[1], "r");

    if (fp == NULL) {
        fprintf(stderr, "\nfopen(%s) failed, errno: %d\n", argv[1], errno);
        return 1;
    }

    fread(pcap_header, PCAP_HEADER_LEN, 1, fp);

    while (1) {

        fread(pkt_header, PKT_HEADER_LEN, 1, fp);

        if (feof(fp)) break;

        packet_len = pkt_header->org_len;

        if (packet_len < 1) break;

        printf("\nPacket number: %d  Frame Len: %d\n\n", packet_num, packet_len);

        eth_start = ftell(fp);

        get_eth_header(fp);

        fseek(fp, eth_start + packet_len, SEEK_SET);

        packet_num++;

//        if (packet_num > 1) break;
    }

    return 0;
}

/* MAC TO STR: MAC Address to String
 * Converts a little-endian uint64_t address to a string with colon formatting
 * (ex: 151138023047680 -> 00:02:2d:90:75:89)
 * */
char *mactostr(uint64_t mac_addr) {

    uint8_t i;
    uint64_t _mac_addr;
    char buff[3] = "", *dst = malloc(18);

    for (i = 0; i < 6; i++) {
        _mac_addr = (mac_addr >> (i * 8)) & 0xff;

//        snprintf(buff, 3, "%02llx", _mac_addr);
        snprintf(buff, 3, "%llx", _mac_addr);
        strncat(dst, buff, 3);

        if (i < 5) {
            strncat(dst, ":", 2);
        }
    }

    return dst;
}

/* IP TO STR: IP Address to String
 * Converts a little-endian uint64_t address to a string with colon formatting
 * (ex: 1711384768 -> 192.168.1.102)
 * */
char *iptostr(uint64_t ip_addr) {

    uint8_t i;
    uint64_t _ip_addr;
    char buff[4] = "", *dst = malloc(18);

    for (i = 0; i < 4; i++) {
        _ip_addr = (ip_addr >> (i * 8)) & 0xff;

        snprintf(buff, 4, "%lld", _ip_addr);
        strncat(dst, buff, 3);

        if (i < 3) {
            strncat(dst, ".", 2);
        }
    }

    return dst;
}

char *get_type(uint8_t protocol, uint16_t code) {

    char *type = malloc(21);

    uint16_t decode = htons(code);

    switch (protocol) {
        case PKT_HEADER:
            printf("PKT: 0x%x \n", decode);
            return "XXX";

        case ETH_HEADER:
            if (decode == 0x0806) return "ARP";
            else if (decode == 0x0800) return "IP";
            break;

        case IPV4_HEADER:
            if (decode == 0x2) return "Reply";
            else if (decode == 0x600) return "TCP";
            else if (decode == 0x1100) return "UDP";
            else if (decode == 0x100) return "ICMP";
            else
                snprintf(type, 8, "Unknown");
            break;

        case ARP_HEADER:
            if (decode == 0x1) return "Request";
            else if (decode == 0x2) return "Reply";
            else if (decode == 0x600) return "TCP";
            else if (decode == 0x1100) return "UDP";
            else if (decode == 0x100) return "ICMP";
            else
                snprintf(type, 20, "%x", decode);
            break;

        case TCP_HEADER:
            if (decode == 0x50) return " HTTP";
            else
                snprintf(type, 20, ": %d", decode);
            break;

        case ICMP_HEADER:
            if (decode == 0x800) return "Request";
            else if (decode == 0x0 || decode == 0x400) return "Reply";
            else
                snprintf(type, 20, "%d", decode);
            break;

        case UDP_HEADER:
            printf("UDP: 0x%x \n", decode);
            break;

        default:
            snprintf(type, 20, "%d", decode);
    }

    return type;
}

void get_eth_header(FILE *fp) {

    uint16_t type;

    eth_header_t *eth_h = malloc(ETH_HEADER_LEN);

    fread(eth_h, ETH_HEADER_LEN, 1, fp);

    type = eth_h->type;

    printf(
            "\tEthernet Header\n"
            "\t\tDest MAC: %s\n"
            "\t\tSource MAC: %s\n"
            "\t\tType: %s\n\n",
            mactostr(eth_h->dst_addr),
            mactostr(eth_h->src_addr),
            get_type(ETH_HEADER, type)
    );

    type = htons(type);

    switch (type) {
        case 0x0806:
            get_arp_header(fp);
            break;
        case 0x0800:
            get_ip_header(fp);
            break;
        default:
//            printf("default main loop case: 0x%02x\n", type);
            printf("default main loop case: 0x%x\n", type);
            break;
    }
}

void get_arp_header(FILE *fp) {

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

void get_ip_header(FILE *fp) {

    char verify[10] = "Correct";

    ip_v4_header_t *ip_header = malloc(ARP_HEADER_LEN);
    fread(ip_header, IPV4_HEADER_LEN, 1, fp);

    uint16_t header_len = (ip_header->Ver_IHL & 0xf) * 4;
    uint8_t protocol = ip_header->protocol;

    // TODO: Fix checksum
    if (in_cksum((unsigned short *) ip_header, IPV4_HEADER_LEN)) {
        snprintf(verify, 10, "Incorrect");
    }

    printf(
            "\tIP Header\n"
            "\t\tHeader Len: %d (bytes)\n"
            //            "\t\tTOS: 0x%02x\n"
            "\t\tTOS: 0x%x\n"
            "\t\tTTL: %d\n"
            "\t\tIP PDU Len: %d (bytes)\n"
            "\t\tProtocol: %s\n"
            //            "\t\tChecksum: %s (0x%04x)\n"
            "\t\tChecksum: %s (0x%x)\n"
            "\t\tSender IP: %s\n"
            "\t\tDest IP: %s\n\n",
            header_len,
            ip_header->DSCP_ECN >> 2,
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
            get_icmp_header(fp);
            break;
        case 0x6:
            get_tcp_header(fp, ip_header);
            break;
        case 0x11:
            get_udp_header(fp);
            break;
        default:
            fprintf(stderr, "IP Protocol Error: 0x%x\n", protocol);
    }
}

unsigned short compute_tcp_checksum(ip_v4_header_t *ip_header, tcp_header_t *tcp_header) {

    register unsigned long sum = 0;
    uint16_t cksum = tcp_header->cksum;

    unsigned short tcp_len = ntohs(ip_header->len) - ((ip_header->Ver_IHL & 0xf) << 2);
    unsigned short *payload = (unsigned short *) tcp_header;

    //add the pseudo header
    //the source ip
    sum += ip_header->src_addr >> 16;
    sum += (ip_header->src_addr) & 0xFFFF;
    //the dest ip
    sum += ip_header->dst_addr >> 16;
    sum += (ip_header->dst_addr) & 0xFFFF;
    //protocol and reserved: 6
    sum += htons(ip_header->protocol);
    sum += htons(ip_header->Flags_Offset);
    //the length
    sum += htons(tcp_len);

    //add the IP payload
    //initialize checksum to 0
    tcp_header->cksum = 0;
    while (tcp_len > 1) {
        sum += *payload++;
        tcp_len -= 2;
    }
    //if any bytes left, pad the bytes and add
    if (tcp_len > 0) {
        //printf("+++++++++++padding, %dn", tcp_len);
        sum += ((*payload) & 0xff);
    }
    //Fold 32-bit sum to 16 bits: add carrier to result
    while (sum >> 16) {
        sum = (sum & 0xffff) + (sum >> 16);
    }
    sum = ~sum;
    //set computation result
    tcp_header->cksum = cksum;
    return (unsigned short) sum;
}

void get_tcp_header(FILE *fp, ip_v4_header_t *ip_header) {

    char ack[21] = "<not valid>";
    char verify[21] = "Correct";

    tcp_header_t *tcp_header = malloc(TCP_HEADER_LEN);

    fread(tcp_header, TCP_HEADER_LEN, 1, fp);

    // TODO: Fix checksum verification
//    if (in_cksum((unsigned short *) tcp_header, TCP_HEADER_LEN)) {
//        snprintf(verify, 20, "Incorrect");
//    }
    unsigned short t = compute_tcp_checksum(ip_header, tcp_header);
    printf("0x%x\n", t);
    if (t) {
        snprintf(verify, 20, "Incorrect");
    }

    uint16_t flags = htons(tcp_header->flags);
    uint32_t ack_num = htonl(tcp_header->ack);

    if ((flags & (0x01 << 4)) != 0) {
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
            // "\t\tChecksum: %s (0x%04x)\n",
            "\t\tChecksum: %s (0x%x)\n",
            get_type(TCP_HEADER, tcp_header->src_port),
            get_type(TCP_HEADER, tcp_header->dst_port),
            htonl(tcp_header->seq),
            ack,
            flags & (0x01 << 4) ? "Yes" : "No",
            flags & (0x01 << 1) ? "Yes" : "No",
            flags & (0x01 << 2) ? "Yes" : "No",
            flags & (0x01 << 0) ? "Yes" : "No",
            htons(tcp_header->win_size),
            verify, htons(tcp_header->cksum)
    );
}

void get_icmp_header(FILE *fp) {

    icmp_header_t *icmp_header = malloc(ICMP_HEADER_LEN);

    fread(icmp_header, ICMP_HEADER_LEN, 1, fp);

    printf(
            "\tICMP Header\n"
            "\t\tType: %s\n",
            get_type(ICMP_HEADER, icmp_header->type)
    );

}

void get_udp_header(FILE *fp) {

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
