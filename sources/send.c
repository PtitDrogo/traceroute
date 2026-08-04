
#include "../includes/ping.h"

void send_packet(struct ping_packet *packet, struct ping_context *ctx) {
    size_t packet_size = sizeof(struct icmphdr) + PAYLOAD_SIZE;
    int err = sendto(ctx->sock, packet, packet_size, 0, ctx->destination_addrinfo->ai_addr,
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

void build_packet(struct ping_packet *packet) {
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

void update_packet(struct ping_packet *packet, uint8_t ttl, uint8_t probe_index) {
    size_t packet_size = sizeof(struct icmphdr) + PAYLOAD_SIZE;
    clock_gettime(CLOCK_REALTIME, (struct timespec *)packet->payload);
    packet->header.un.echo.sequence = htons(1);
    packet->header.un.echo.sequence = htons((uint16_t)(ttl * 10 + probe_index));
    packet->header.checksum = 0;
    packet->header.checksum = checksum((uint16_t *)packet, packet_size);
}

void update_ping_info(double time_rtt, struct ping_context *ctx) {
    (void)time_rtt;
    (void)ctx;
    // nothing here ...
}

void cleanup(struct ping_context *ctx) {
    if (ctx->destination_addrinfo)
        freeaddrinfo(ctx->destination_addrinfo);
    if (ctx->sock >= 0)
        close(ctx->sock);
    restore_termios();
}