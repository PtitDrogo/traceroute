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


    struct probe_record probes[MAX_TTL][MAX_PROBES];
    */
    // We send the first few hops.

    // I have 5 probes, max probes is 3
    // probes 0,1,2 go to index 1
    // probes 3,4 go to index 2
    //  14 / 3 = 4
    //  0, 1 ,2
    //  4, 5 -
    uint8_t send_i = 0;
    for (; send_i < 16; send_i++) {
        uint8_t ttl = send_i / MAX_PROBES; // TTL 1 will be stored at index 0, Im sure this wont be confusing at all ...
        uint8_t probe_index = send_i % MAX_PROBES;
        update_socket(&ctx, ttl + 1);
        update_packet(&packet, ttl, probe_index);
        send_packet(&packet, &ctx);
        clock_gettime(CLOCK_REALTIME, &ctx.probes[ttl][probe_index].sent_at);
    }

    struct probe_index oldest_i = {0};

    struct pollfd fd;
    fd.events = POLLIN;
    fd.fd = ctx.sock;

    // In theory this loop doesnt actually give a single shit about where we are at in an index.
    // when It sees a response, its update the big struct, then decides to send a new one.
    while (true) {
        int err = poll(&fd, 1, 1000);
        if (err < 0) {
            printf("traceroute: poll error\n");
            cleanup(&ctx);
            return EXIT_FAILURE;
        }
        // Logic is, I dont wanna lose any time responding to actual responses.
        if (fd.revents & POLLIN) {
            handle_reply(&ctx);
            if (ctx.probes[oldest_i.ttl][oldest_i.probe].status == RESPONDED) {
                // printf("We overwrote the oldest probe oh no\n");
            }
        } else {
            uint8_t available_probe_slots = timeout_outdated_probes(&ctx, &oldest_i);
            for (uint8_t i = 0; i < available_probe_slots; i++) {
                uint8_t ttl = send_i / MAX_PROBES;
                uint8_t probe_index = send_i % MAX_PROBES;
                update_socket(&ctx, ttl + 1);
                update_packet(&packet, ttl, probe_index);
                send_packet(&packet, &ctx);
                clock_gettime(CLOCK_REALTIME, &ctx.probes[ttl][probe_index].sent_at);
                send_i += 1;
            }
        }

        // Now I need to send as many probes as were timeouted.
    }

    // In theory we never get here
    cleanup(&ctx);
    return EXIT_FAILURE; // This is actually a failure if we get here.
}

static void init(struct ping_context *ctx) { (void)ctx; }