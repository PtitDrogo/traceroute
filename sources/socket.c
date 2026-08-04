#include "../includes/ping.h"

void create_socket(struct options options, struct ping_context *ctx) {
    int sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (options.dont_route) {
        int enable = 1;
        if (setsockopt(sock, SOL_SOCKET, SO_DONTROUTE, &enable, sizeof(enable)) < 0) {
            perror("ping: setsockopt SO_DONTROUTE");
            cleanup(ctx);
            exit(1);
        }
    }
    if (options.ttl) {
        if (setsockopt(sock, IPPROTO_IP, IP_TTL, &options.ttl, sizeof(options.ttl)) < 0) {
            perror("ping: setsockopt IP_TTL");
            cleanup(ctx);
            exit(1);
        }
    }
    ctx->sock = sock;
    if (options.verbose)
        print_socket_verbose(ctx);
    return;
}