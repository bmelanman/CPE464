
#include "trace.h"

int main(int argc, char *argv[]) {

    uint32_t pkt_num = 1;
    uint8_t *packet_data = NULL;

    char errbuf[PCAP_ERRBUF_SIZE];

    pcap_t *fp;
    struct pcap_pkthdr *pcap_header = malloc(PCAP_HEADER_LEN);

    /* Check for single input */
    if (argc != 2) {
        fprintf(stderr, "ERR: Please provide an input *.pcap file!\n");
        return 1;
    }

    /* Open the pcap file */
    fp = pcap_open_offline(argv[1], errbuf);

    /* Verify file pointer */
    if (fp == NULL) {
        fprintf(stderr, "\npcap_open_offline() failed: %s\n", errbuf);
        return 1;
    }

    /* Iterate through each packet */
    while (pcap_next_ex(fp, &pcap_header, (const u_char **) &packet_data) >= 0) {

        /* Display packet info */
        printf("\nPacket number: %d  Frame Len: %d\n\n", pkt_num++, pcap_header->len);

        /* Begin parsing eth packet */
        process_eth_h(packet_data);
    }

    /* Clean up */
    pcap_close(fp);
    free(pcap_header);

    return 0;
}

char *mactostr(uint8_t mac_addr[6]) {

    char *mac_str = malloc(19);

    snprintf(mac_str, 18, "%s", ether_ntoa((const struct ether_addr *) mac_addr));

    return mac_str;
}

char *iptostr(uint32_t ip_addr) {

    char *str_addr = malloc(16);

    inet_ntop(AF_INET, &(ip_addr), str_addr, INET_ADDRSTRLEN);

    return str_addr;
}

char *get_type(uint8_t protocol, uint16_t type) {

    char *unknown_type;

    switch (protocol) {
        case ETH_HEADER:
        case IPV4_HEADER:
            switch (type) {
                case 0x0008:
                    return "IP";
                case 0x0608:
                    return "ARP";
                case 0x0006:
                    return "TCP";
                case 0x0011:
                    return "UDP";
                case 0x0001:
                    return "ICMP";
                default:
                    return strdup("Unknown");
            }

        case ARP_HEADER:
            switch (type) {
                case 0x0100:
                    return "Request";
                case 0x0200:
                    return "Reply";
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

void process_eth_h(uint8_t *packet_data) {

    /* Copy header data into a struct */
    eth_header_t *eth_header = malloc(ETH_HEADER_LEN);
    memcpy(eth_header, packet_data, ETH_HEADER_LEN);

    /* Advance the packet data pointer to the next header */
    packet_data = &packet_data[ETH_HEADER_LEN];

    uint16_t type = eth_header->type;

    /* Parse header */
    printf(
            "\tEthernet Header\n"
            "\t\tDest MAC: %s\n"
            "\t\tSource MAC: %s\n"
            "\t\tType: %s\n\n",
            mactostr(eth_header->dst_addr),
            mactostr(eth_header->src_addr),
            get_type(ETH_HEADER, type)
    );

    switch (type) {
        case 0x0608:
            process_arp_h(packet_data);
            break;
        case 0x0008:
            process_ip_h(packet_data);
            break;
        default:
            break;
    }

}

void process_arp_h(uint8_t *packet_data) {

    arp_header_t *arp_header = malloc(ARP_HEADER_LEN);

    memcpy(arp_header, packet_data, ARP_HEADER_LEN);

    printf(
            "\tARP header\n"
            "\t\tOpcode: %s\n"
            "\t\tSender MAC: %s\n"
            "\t\tSender IP: %s\n"
            "\t\tTarget MAC: %s\n"
            "\t\tTarget IP: %s\n\n",
            get_type(ARP_HEADER, arp_header->OPER),
            mactostr(arp_header->SHA), iptostr(arp_header->SPA),
            mactostr(arp_header->THA), iptostr(arp_header->TPA)
    );
}

void process_ip_h(uint8_t *packet_data) {

    char verify[10] = "Correct";

    ip_v4_header_t *ip_header = malloc(IPV4_HEADER_LEN);
    pseudo_header_t *pseudo_header = malloc(PSEUDO_HEADER_LEN);

    /* Move packet data into a struct */
    memcpy(ip_header, packet_data, IPV4_HEADER_LEN);

    uint16_t header_len = (ip_header->Ver_IHL & 0xf) * 4;
    uint8_t protocol = ip_header->protocol;

    /* Verify checksum */
    if (in_cksum((unsigned short *) packet_data, header_len)) {
        snprintf(verify, 10, "Incorrect");
    }

    /* Advance the packet data pointer to the next header */
    packet_data = &packet_data[header_len];

    /* Unpack data */
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

    /* Go to the next header unpacker */
    switch (protocol) {
        case 0x1:
            process_icmp_h(packet_data);
            break;
        case 0x6:
            pseudo_header->src_addr = ip_header->src_addr;
            pseudo_header->dst_addr = ip_header->dst_addr;
            pseudo_header->type = 0x0600;
            pseudo_header->tcp_len = ntohs(htons(ip_header->len) - header_len);
            process_tcp_h(packet_data, pseudo_header);
            break;
        case 0x11:
            process_udp_h(packet_data);
            break;
        default:
            break;
    }
}

void process_tcp_h(uint8_t *packet_data, pseudo_header_t *pseudo_header) {

    uint16_t tcp_pkt_len = htons(pseudo_header->tcp_len);

    char ack[21] = "<not valid>";
    char verify[21] = "Correct";

    tcp_header_t *tcp_header = malloc(TCP_HEADER_LEN);

    memcpy(tcp_header, packet_data, TCP_HEADER_LEN);

    unsigned short *checksum_packet = malloc(PSEUDO_HEADER_LEN + tcp_pkt_len);
    memcpy(checksum_packet, pseudo_header, PSEUDO_HEADER_LEN);
    memcpy(&checksum_packet[6], packet_data, tcp_pkt_len);

    /* TODO: Fix checksum */
    if (in_cksum(checksum_packet, PSEUDO_HEADER_LEN + tcp_pkt_len)) {
        snprintf(verify, 20, "Incorrect");
    }

    uint16_t flags = htons(tcp_header->flags);
    uint32_t ack_num = htonl(tcp_header->ack);
    uint8_t ack_flag = flags & (0x10);

    if (ack_flag) {
        snprintf(ack, 20, "%u", ack_num);
    }

    printf(
            "\n"
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
            flags & (0x02) ? "Yes" : "No",
            flags & (0x04) ? "Yes" : "No",
            flags & (0x01) ? "Yes" : "No",
            htons(tcp_header->win_size),
            verify, htons(tcp_header->cksum)
    );
}

void process_icmp_h(uint8_t *packet_data) {

    icmp_header_t *icmp_header = malloc(ICMP_HEADER_LEN);

    memcpy(icmp_header, packet_data, ICMP_HEADER_LEN);

    printf(
            "\n"
            "\tICMP Header\n"
            "\t\tType: %s\n",
            get_type(ICMP_HEADER, icmp_header->type)
    );

}

void process_udp_h(uint8_t *packet_data) {

    udp_header_t *udp_header = malloc(UDP_HEADER_LEN);

    memcpy(udp_header, packet_data, UDP_HEADER_LEN);

    printf(
            "\n"
            "\tUDP Header\n"
            "\t\tSource Port: : %u\n"
            "\t\tDest Port: : %u\n",
            htons(udp_header->src_port),
            htons(udp_header->dst_port)
    );

}
