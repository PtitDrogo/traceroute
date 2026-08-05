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

    /*
    int OlDestProbePointerIndex;


    for 16:
    send()

    while(True) {
    poll(recvSocket);
    //We dont actually
    //care about the timeout
    //value of poll here
    //Timeout is only so we
    //can exit if no response.
    if (POLLIN)
        respon.seq / 3 %3 -> probe.responded

    if (probe == oldest)
        print()
        while(notResponded or end);
            oldestProbe += 1

    if TimeNow - OldestProbe.timesent > Timeout
        OldestProbe.status = ignored.
        printf("*"); if probe.index == maxprobe -> printf("\n");
        while(notResponded or end);
            oldestProbe += 1;

    //We only send a new probe if we actually handled one
    //Right before
    //We are only handling the probes one by one, so this actually works
    //flawlessly, the number of probes sent at the beginning becomes the rule.
    //Easy flag
    if poll != -1
        send()

    }

    */

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