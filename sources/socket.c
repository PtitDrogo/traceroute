#include "../includes/ping.h"

void create_socket(struct options options, struct ping_context *ctx) {
    // For now were using ICMP, but later the protocol will be a variable.
    (void)options;
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ctx->sock = sock;
    return;
}

void update_socket(struct ping_context *ctx, uint8_t ttl) {
    if (setsockopt(ctx->sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
        perror("traceroute: setsockopt IP_TTL");
        cleanup(ctx);
        exit(1);
    }
}