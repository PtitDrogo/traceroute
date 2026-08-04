#include "../includes/ping.h"

static void init(struct ping_context *ctx);

int main(int argc, char *argv[]) {
    if (geteuid() != 0) {
        printf("ping: sudo rights are required, exiting.\n");
        return 1;
    }
    struct ping_context ctx = {0};
    init(&ctx);
    disable_echoctl();
    parse_flags(&ctx, argc, argv);
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;

    create_socket(ctx.options, &ctx);
    int err = getaddrinfo(ctx.res.arg_address, NULL, &hints, &ctx.destination_addrinfo);
    if (err != 0) {
        fprintf(stderr, "ping: %s: Name or service not known\n", ctx.res.arg_address);
        cleanup(&ctx);
        return 1;
    }

    struct ping_packet packet = {.header.type = ICMP_ECHO,
                                 .header.code = 0,
                                 .header.checksum = 0,
                                 .header.un = {.echo = {.id = htons(getpid()), .sequence = htons(ctx.seq)}}};
    build_packet(&packet, &ctx);
    clock_gettime(CLOCK_REALTIME, &ctx.res.start_time);

    for (uint8_t i = 1; i < DEFAULT_MAX_TTL; i++) {
        update_socket(&ctx, i);
        send_packet(&packet, &ctx);
        handle_reply(&ctx);
    }

    cleanup(&ctx);
    return EXIT_FAILURE; // This is actually a failure if we get here.
}

static void init(struct ping_context *ctx) { ctx->seq = 1; }