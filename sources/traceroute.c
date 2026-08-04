#include "../includes/traceroute.h"

static void init(struct ping_context *ctx);

int main(int argc, char *argv[]) {
    if (geteuid() != 0) {
        printf("ping: sudo rights are required, exiting.\n");
        return EXIT_FAILURE;
    }

    struct ping_context ctx = {0};
    init(&ctx);
    disable_echoctl();
    parse_flags(&ctx, argc, argv);
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;

    create_socket(ctx.options, &ctx);
    int err = getaddrinfo(ctx.arg_address, NULL, &hints, &ctx.destination_addrinfo);
    if (err != 0) {
        fprintf(stderr, "ping: %s: Name or service not known\n", ctx.arg_address);
        cleanup(&ctx);
        return 1;
    }

    struct ping_packet packet = {.header.type = ICMP_ECHO,
                                 .header.code = 0,
                                 .header.checksum = 0,
                                 .header.un = {.echo = {.id = htons(getpid()), .sequence = htons(1)}}};
    build_packet(&packet);
    clock_gettime(CLOCK_REALTIME, &ctx.res.start_time);

    print_start_string(ctx);
    for (uint8_t ttl = 1; ttl < DEFAULT_TTL; ttl++) {
        for (uint8_t probe_index = 0; probe_index < 1; probe_index++) { // will be modifiable after.
            update_socket(&ctx, ttl);
            update_packet(&packet, ttl, probe_index);

            clock_gettime(CLOCK_REALTIME, &ctx.probes[ttl][probe_index].sent_at);
            ctx.probes[ttl][probe_index].ttl = ttl;
            ctx.probes[ttl][probe_index].replied = false;

            send_packet(&packet, &ctx);
            handle_reply(&ctx);
        }
    }

    cleanup(&ctx);
    return EXIT_FAILURE; // This is actually a failure if we get here.
}

static void init(struct ping_context *ctx) { (void)ctx; }