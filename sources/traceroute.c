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

    create_socket(&ctx);
    int err = getaddrinfo(ctx.arg_address, NULL, &hints, &ctx.destination_addrinfo);
    if (err != 0) {
        fprintf(stderr, "ping: %s: Name or service not known\n", ctx.arg_address);
        cleanup(&ctx);
        return 1;
    }

    icmp_packet_t icmp_packet = {.header.type = ICMP_ECHO,
                            .header.code = 0,
                            .header.checksum = 0,
                            .header.un = {.echo = {.id = htons(getpid()), .sequence = htons(1)}}};
    build_packet(&icmp_packet);
    udp_packet_t udp_packet;

    print_start_string(&ctx);
    uint32_t send_i = 0;
    if (ctx.options.use_icmp) {
        for (; send_i < ctx.options.max_probes_in_flight; send_i++) {
            icmp_sending_protocol(send_i, &ctx, &icmp_packet);
        }
    } else {
        for (; send_i < ctx.options.max_probes_in_flight; send_i++) {
           udp_sending_protocol(send_i, &ctx, &udp_packet);
        }
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
        if (ctx.options.use_icmp) {
            for (uint8_t i = 0; i < available_probe_slots; i++) {
                icmp_sending_protocol(send_i, &ctx, &icmp_packet);
                send_i += 1;
            }
        } else {
            for (uint8_t i = 0; i < available_probe_slots; i++) {
                udp_sending_protocol(send_i, &ctx, &udp_packet);
                send_i += 1;
            }
        }

        print_ready_ttl_groups(&ctx);
    }

    // In theory we never get here
    cleanup(&ctx);
    return EXIT_FAILURE; // This is actually a failure if we get here.
}

static void init(ping_context_t *ctx) {
    ctx->final_ttl = DEFAULT_TTL;
    ctx->options.max_probes_per_ttl = DEFAULT_PROBE;
    ctx->options.max_probes_in_flight = DEFAULT_IN_FLIGHT_PROBES;
} // This wont even proc here, it will be handled during the parsing of the arguments D: