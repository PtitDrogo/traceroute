#include "../includes/traceroute.h"

static ping_context_t init();

int main(int argc, char *argv[]) {
    if (geteuid() != 0) {
        printf("ping: sudo rights are required, exiting.\n");
        return EXIT_FAILURE;
    }
    ping_context_t ctx = init();
    parse_flags(&ctx, argc, argv);
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;

    create_socket(&ctx);
    int err = getaddrinfo(ctx.arg_address, NULL, &hints, &ctx.destination_addrinfo);
    if (err != 0) {
        fprintf(stderr, "ping: %s: Name or service not known\n", ctx.arg_address);
        cleanup(&ctx);
        return 1;
    }

    icmp_packet_t icmp_packet = build_icmp_packet();
    char *udp_payload[UINT16_MAX];

    print_start_string(&ctx);
    uint32_t send_i = 0;
    void *packet = ctx.options.use_icmp ? (void *)&icmp_packet : (void *)&udp_payload;
    for (; send_i < ctx.options.max_probes_in_flight; send_i++) {
        send_protocol(send_i, &ctx, packet);
    }

    probe_index_t oldest_i = {0};

    struct pollfd fd;
    fd.events = POLLIN;
    fd.fd = ctx.recv_sock;

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
            printf("\n\n Sending new probes ! \n\n");
            send_protocol(send_i, &ctx, packet);
            send_i++;
        }
        // print_probe_struct(&ctx);

        print_ready_ttl_groups(&ctx);
    }

    // In theory we never get here
    cleanup(&ctx);
    return EXIT_FAILURE; // This is actually a failure if we get here.
}

static ping_context_t init() {
    disable_echoctl();
    ping_context_t ctx = {0};
    ctx.final_ttl_index = DEFAULT_TTL - 1;
    ctx.options.max_probes_per_ttl = DEFAULT_PROBE;
    ctx.options.max_probes_in_flight = DEFAULT_IN_FLIGHT_PROBES;
    return ctx;
}