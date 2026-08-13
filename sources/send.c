
#include "../includes/traceroute.h"

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

icmp_packet_t build_icmp_packet() {
    icmp_packet_t icmp_packet = {.header.type = ICMP_ECHO,
                                 .header.code = 0,
                                 .header.checksum = 0,
                                 .header.un = {.echo = {.id = htons(getpid()), .sequence = htons(1)}}};
    size_t packet_size = sizeof(struct icmphdr) + PAYLOAD_SIZE;
    size_t start_idx = 0;
    clock_gettime(CLOCK_REALTIME, (struct timespec *)icmp_packet.payload);
    start_idx = sizeof(struct timespec);

    for (size_t i = start_idx; i < PAYLOAD_SIZE; i++) {
        icmp_packet.payload[i] = i;
    }

    icmp_packet.header.checksum = 0;
    icmp_packet.header.checksum = checksum((uint16_t *)&icmp_packet, packet_size);
    return icmp_packet;
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

void update_packet(void *packet, probe_index_t index, ping_context_t *ctx) {
    if (ctx->options.use_icmp) {
        icmp_packet_t *icmp = (icmp_packet_t *)packet;
        size_t packet_size = sizeof(struct icmphdr) + PAYLOAD_SIZE;
        clock_gettime(CLOCK_REALTIME, (struct timespec *)icmp->payload);
        icmp->header.un.echo.sequence = htons(1);
        icmp->header.un.echo.sequence = htons((uint16_t)(index.ttl * 10 + index.probe));
        icmp->header.checksum = 0;
        icmp->header.checksum = checksum((uint16_t *)icmp, packet_size);

        printf("ready to send packet: sequence :%d, ", ntohs(icmp->header.un.echo.sequence));
        printf("ttl i: %d, probe i: %d\n", get_probe_index_from_sequence(ntohs(icmp->header.un.echo.sequence)).ttl,
               get_probe_index_from_sequence(ntohs(icmp->header.un.echo.sequence)).probe);
    } else {
        char *udp_payload = (char *)packet;
        struct sockaddr_in *dest = (struct sockaddr_in *)ctx->destination_addrinfo->ai_addr;
        clock_gettime(CLOCK_REALTIME, (struct timespec *)udp_payload);
        dest->sin_port = htons(BASE_UDP_PORT + (index.ttl * MAX_PROBES + index.probe));
    }
}

void send_packet(void *packet, ping_context_t *ctx) {
    size_t packet_size = ctx->options.use_icmp ? sizeof(struct icmphdr) + PAYLOAD_SIZE : PAYLOAD_SIZE;

    int err = sendto(ctx->send_sock, packet, packet_size, 0, ctx->destination_addrinfo->ai_addr,
                     ctx->destination_addrinfo->ai_addrlen);
    if (err == -1) {
        fprintf(stderr, "ping: sendto: %s\n", strerror(errno));
        cleanup(ctx);
        exit(1);
    }
}

void send_protocol(uint8_t send_i, ping_context_t *ctx, void *packet) {

    probe_index_t index = {.ttl = send_i / ctx->options.max_probes_per_ttl,
                           .probe = send_i % ctx->options.max_probes_per_ttl};

    update_socket(ctx, index.ttl + 1);
    update_packet(packet, index, ctx);
    send_packet(packet, ctx);
    printf("i ttl: %d, i probe: %d\n", index.ttl, index.probe);
    clock_gettime(CLOCK_REALTIME, &ctx->probes[index.ttl][index.probe].sent_at);
    printf("Launched at %ld\n", ctx->probes[index.ttl][index.probe].sent_at.tv_sec);
}