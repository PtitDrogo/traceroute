#include "../includes/traceroute.h"

static void parse_udp_reply(char *response, struct icmphdr **outer_icmp, struct udphdr **reply_udp) {
    struct iphdr *ip = (struct iphdr *)response;
    int ip_hdr_len = ip->ihl * 4;
    *outer_icmp = (struct icmphdr *)(response + ip_hdr_len);

    // UDP probes only ever get ICMP *error* replies back (never a "success" reply type),
    // so the inner original packet is always present under TIME_EXCEEDED / DEST_UNREACH.
    struct iphdr *inner_ip = (struct iphdr *)(response + ip_hdr_len + sizeof(struct icmphdr));
    int inner_ip_hdr_len = inner_ip->ihl * 4;
    *reply_udp = (struct udphdr *)((char *)inner_ip + inner_ip_hdr_len);
}

int main(int argc, char *argv[]) {
    // Create a udp socket using DGRAM
    struct addrinfo *destination_addrinfo;

    int icmp_sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
    setsockopt(icmp_sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    int udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;

    int err = getaddrinfo("discord.com", NULL, &hints, &destination_addrinfo);
    if (err != 0) {
        fprintf(stderr, "ping:s: Name or service not known\n");
        return 1;
    }

    // Setting TTL to whats given in the argument
    int ttl = atoi(argv[1]);
    if (setsockopt(udp_sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
        perror("traceroute: setsockopt IP_TTL");
        exit(1);
    }

    struct sockaddr_in *dest = (struct sockaddr_in *)destination_addrinfo->ai_addr;
    dest->sin_port = htons(BASE_UDP_PORT + 89); // BASE_UDP_PORT = 33434
    char udp_payload[PAYLOAD_SIZE] = {0};
    // Send
    err = sendto(udp_sock, udp_payload, sizeof(udp_payload), 0, (struct sockaddr *)destination_addrinfo->ai_addr,
                 destination_addrinfo->ai_addrlen);
    if (err == -1) {
        fprintf(stderr, "ping: sendto: %s\n", strerror(errno));
        exit(1);
    }

    struct sockaddr_in response_in;
    socklen_t src_len = sizeof(response_in);
    char response[UINT16_MAX];

    ssize_t bytes_received =
        recvfrom(icmp_sock, &response, sizeof(response), 0, (struct sockaddr *)&response_in, &src_len);
    if (bytes_received == -1) {
        exit(1);
    }
    char *ip_addr = inet_ntoa(response_in.sin_addr);

    struct udphdr *reply_udp;
    struct icmphdr *outer_icmp;
    icmp_packet_t *reply;

    parse_udp_reply(response, &outer_icmp, &reply_udp);

    printf("hop reply from %s: icmp_type=%d icmp_code=%d (%s) "
           "orig_dst_port=%d ip_id=%d ip_ttl=%d icmp_seq/id? none (UDP probe)\n",
           ip_addr, outer_icmp->type, outer_icmp->code,
           outer_icmp->type == ICMP_TIME_EXCEEDED  ? "Time Exceeded"
           : outer_icmp->type == ICMP_DEST_UNREACH ? "Dest Unreachable"
                                                   : "Other",
           ntohs(reply_udp->dest), ntohs(((struct iphdr *)response)->id), ((struct iphdr *)response)->ttl);
}