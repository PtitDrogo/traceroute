
#include "../includes/traceroute.h"

void icmp_sending_protocol(uint8_t send_i, ping_context_t *ctx, icmp_packet_t *packet) {
    uint8_t ttl = send_i / ctx->options.max_probes_per_ttl;
    uint8_t probe_index = send_i % ctx->options.max_probes_per_ttl;
    update_socket(ctx, ttl + 1);
    update_icmp_packet(packet, ttl, probe_index);
    send_icmp_packet(packet, ctx);
    clock_gettime(CLOCK_REALTIME, &ctx->probes[ttl][probe_index].sent_at);
}

void send_icmp_packet(icmp_packet_t *packet, ping_context_t *ctx) {
    size_t packet_size = sizeof(struct icmphdr) + PAYLOAD_SIZE;
    int err = sendto(ctx->send_sock, packet, packet_size, 0, ctx->destination_addrinfo->ai_addr,
                     ctx->destination_addrinfo->ai_addrlen);
    if (err == -1) {
        fprintf(stderr, "ping: sendto: %s\n", strerror(errno));
        cleanup(ctx);
        exit(1);
    }
}

void send_udp_packet(udp_packet_t *packet, ping_context_t *ctx) {
    size_t packet_size = sizeof(struct udphdr) + PAYLOAD_SIZE;
    int err = sendto(ctx->send_sock, packet, packet_size, 0, ctx->destination_addrinfo->ai_addr,
                     ctx->destination_addrinfo->ai_addrlen);
    if (err == -1) {
        fprintf(stderr, "ping: sendto: %s\n", strerror(errno));
        cleanup(ctx);
        exit(1);
    }
}

// This write the proper MaybeResolvedAddressString (ip string) to dest buf
void get_hostname_string_from_ip(const char *ip_str, char *dest_buf, size_t buf_len) {
    if (!ip_str || !dest_buf || buf_len == 0)
        return;

    struct sockaddr_in sa = {0};
    sa.sin_family = AF_INET;
    if (inet_pton(AF_INET, ip_str, &sa.sin_addr) <= 0)
        return;

    char host[NI_MAXHOST];
    getnameinfo((struct sockaddr *)&sa, sizeof(sa), host, sizeof(host), NULL, 0, 0);
    snprintf(dest_buf, buf_len, "%s (%s)", host, ip_str);
    return;
}

// returns the time difference between the given timespec and now.
double compute_time_difference(struct timespec past_time) {
    struct timespec end_time;
    clock_gettime(CLOCK_REALTIME, &end_time);

    const struct timespec start_time = past_time;

    double rtt = (end_time.tv_sec - start_time.tv_sec) * 1000.0 + (end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;
    return rtt;
}

void build_packet(icmp_packet_t *packet) {
    size_t packet_size = sizeof(struct icmphdr) + PAYLOAD_SIZE;
    size_t start_idx = 0;
    clock_gettime(CLOCK_REALTIME, (struct timespec *)packet->payload);
    start_idx = sizeof(struct timespec);

    for (size_t i = start_idx; i < PAYLOAD_SIZE; i++) {
        packet->payload[i] = i;
    }

    packet->header.un.echo.sequence = htons(1);
    packet->header.checksum = 0;
    packet->header.checksum = checksum((uint16_t *)packet, packet_size);
}

void update_icmp_packet(icmp_packet_t *packet, uint8_t ttl, uint8_t probe_index) {
    size_t packet_size = sizeof(struct icmphdr) + PAYLOAD_SIZE;
    clock_gettime(CLOCK_REALTIME, (struct timespec *)packet->payload);
    packet->header.un.echo.sequence = htons(1);
    packet->header.un.echo.sequence = htons((uint16_t)(ttl * 10 + probe_index));
    packet->header.checksum = 0;
    packet->header.checksum = checksum((uint16_t *)packet, packet_size);
};

// Here, Since we are sending to a specific port, we actually update the info of port in the sockadrr_in we will send to
// We still can upload the payload with the time AFAIK
void update_udp_packet(char *payload, struct sockaddr_in *dest_addr, uint8_t ttl, uint8_t probe_index) {
    clock_gettime(CLOCK_REALTIME, (struct timespec *)payload);
    dest_addr->sin_port = htons(BASE_UDP_PORT + (ttl * MAX_PROBES + probe_index));
}

void cleanup(ping_context_t *ctx) {
    if (ctx->destination_addrinfo)
        freeaddrinfo(ctx->destination_addrinfo);
    if (ctx->send_sock >= 0)
        close(ctx->send_sock);
    if (ctx->recv_sock >= 0)
        close(ctx->recv_sock);
    restore_termios();
}