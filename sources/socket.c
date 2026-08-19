#include "../includes/traceroute.h"

void create_socket(tr_context_t *ctx) {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    ctx->recv_sock = sock;

    if (ctx->options.use_icmp) {
        ctx->send_sock = sock;
        if (ctx->options.skip_routing) {
            int enable = 1;
            if (setsockopt(ctx->send_sock, SOL_SOCKET, SO_DONTROUTE, &enable, sizeof(enable)) < 0) {
                perror("traceroute: setsockopt SO_DONTROUTE");
                cleanup(ctx);
                exit(1);
            }
        }
        return;
    }
    int udp_sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    ctx->send_sock = udp_sock;
    if (ctx->options.skip_routing) {
        int enable = 1;
        if (setsockopt(ctx->send_sock, SOL_SOCKET, SO_DONTROUTE, &enable, sizeof(enable)) < 0) {
            perror("traceroute: setsockopt SO_DONTROUTE");
            cleanup(ctx);
            exit(1);
        }
    }
    return;
}

void update_socket(tr_context_t *ctx, uint8_t ttl) {
    if (setsockopt(ctx->send_sock, IPPROTO_IP, IP_TTL, &ttl, sizeof(ttl)) < 0) {
        perror("traceroute: setsockopt IP_TTL");
        cleanup(ctx);
        exit(1);
    }
}