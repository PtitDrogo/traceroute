#include "../includes/traceroute.h"

static void init(tr_context_t *ctx);

int main(int argc, char *argv[]) {
    if (geteuid() != 0) {
        printf("traceroute: sudo rights are required, exiting.\n");
        return EXIT_FAILURE;
    }
    tr_context_t ctx = {0};
    init(&ctx);
    parse_flags(&ctx, argc, argv);
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;
    hints.ai_socktype = ctx.options.use_icmp ? SOCK_RAW : SOCK_DGRAM;

    create_socket(&ctx);
    int err = getaddrinfo(ctx.arg_address, NULL, &hints, &ctx.destination_addrinfo);
    if (err != 0) {
        fprintf(stderr, "traceroute: %s: Name or service not known\n", ctx.arg_address);
        cleanup(&ctx);
        return 1;
    }

    icmp_packet_t icmp_packet = build_icmp_packet();
    char udp_payload[PAYLOAD_SIZE] = {0};

    print_start_string(&ctx);
    uint32_t send_i = 0;
    void *packet = ctx.options.use_icmp ? (void *)&icmp_packet : (void *)udp_payload;
    for (; send_i < ctx.options.max_probes_in_flight; send_i++) {
        send_protocol(send_i, &ctx, packet);
        ctx.time_slept_ms += sleep_and_measure(ctx.options.time_to_sleep_ms);
    }

    probe_index_t oldest_i = {0};

    struct pollfd fd;
    fd.events = POLLIN;
    fd.fd = ctx.recv_sock;

    while (true) {
        int err = poll(&fd, 1, 100);
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
            send_protocol(send_i, &ctx, packet);
            ctx.time_slept_ms += sleep_and_measure(ctx.options.time_to_sleep_ms);
            send_i++;
        }

        print_ready_ttl_groups(&ctx);
    }

    cleanup(&ctx);
    return EXIT_FAILURE;
}

static void init(tr_context_t *ctx) {
    ctx->final_ttl_index = DEFAULT_TTL - 1;
    ctx->options.max_probes_per_ttl = DEFAULT_PROBE;
    ctx->options.max_probes_in_flight = DEFAULT_IN_FLIGHT_PROBES;
    ctx->options.time_to_sleep_ms = SLEEP_TIME_MS;
}