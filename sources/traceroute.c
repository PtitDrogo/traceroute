#include "../includes/traceroute.h"

static void init(ping_context_t *ctx);

int main(int argc, char *argv[]) {
    if (geteuid() != 0) {
        printf("ping: sudo rights are required, exiting.\n");
        return EXIT_FAILURE;
    }

    ping_context_t ctx = {0};
    disable_echoctl();
    init(&ctx);
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

    ping_packet_t packet = {.header.type = ICMP_ECHO,
                            .header.code = 0,
                            .header.checksum = 0,
                            .header.un = {.echo = {.id = htons(getpid()), .sequence = htons(1)}}};
    build_packet(&packet);
    // clock_gettime(CLOCK_REALTIME, &ctx.res.start_time);

    print_start_string(&ctx);
    uint8_t send_i = 0;
    for (; send_i < 16; send_i++) {
        uint8_t ttl = send_i / ctx.options.max_probes; // TTL 1 will be stored at index 0, Im sure this wont be confusing at all ...
        uint8_t probe_index = send_i % ctx.options.max_probes;
        update_socket(&ctx, ttl + 1);
        update_packet(&packet, ttl, probe_index);
        send_packet(&packet, &ctx);
        clock_gettime(CLOCK_REALTIME, &ctx.probes[ttl][probe_index].sent_at);
    }

    probe_index_t oldest_i = {0};

    struct pollfd fd;
    fd.events = POLLIN;
    fd.fd = ctx.sock;

    while (true) {
        int err = poll(&fd, 1, 1000);
        if (err < 0) {
            printf("traceroute: poll error\n");
            cleanup(&ctx);
            return EXIT_FAILURE;
        }
        if (fd.revents & POLLIN) {
            handle_reply(&ctx);
        }
        uint8_t available_probe_slots = handle_responded_probes(&ctx, &oldest_i);
        for (uint8_t i = 0; i < available_probe_slots; i++) {
            // printf("sending, send_i = %d\n", send_i);
            uint8_t ttl = send_i / ctx.options.max_probes;
            uint8_t probe_index = send_i % ctx.options.max_probes;
            update_socket(&ctx, ttl + 1);
            update_packet(&packet, ttl, probe_index);
            send_packet(&packet, &ctx);
            clock_gettime(CLOCK_REALTIME, &ctx.probes[ttl][probe_index].sent_at);
            send_i += 1;
        }

        print_ready_ttl_groups(&ctx);
    }

    // In theory we never get here
    cleanup(&ctx);
    return EXIT_FAILURE; // This is actually a failure if we get here.
}

static void init(ping_context_t *ctx) {
    ctx->final_ttl = DEFAULT_TTL;
    ctx->options.max_probes = DEFAULT_PROBE;
} // This wont even proc here, it will be handled during the parsing of the arguments D: