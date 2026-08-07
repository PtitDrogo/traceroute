#include "../includes/traceroute.h"

void create_socket(ping_context_t *ctx) {
    // For now were using ICMP, but later the protocol will be a variable.
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ctx->recv_sock = sock;

    if (ctx->options.use_icmp) {
        ctx->send_sock = sock;
        return;
    }
    printf("Creating udp socket\n");
    int udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ctx->send_sock = udp_sock;
    return;
}

void update_socket(ping_context_t *ctx, uint8_t ttl) {
    printf("Send sock: %d\n", ctx->send_sock);
    if (setsockopt(ctx->send_sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
        perror("traceroute: setsockopt IP_TTL");
        cleanup(ctx);
        exit(1);
    }
}