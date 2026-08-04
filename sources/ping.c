#include "../includes/ping.h"

static void handle_sig(int sig);
static void install_handlers(void);
static void init(struct ping_context *ctx);

struct state state;

int main(int argc, char *argv[]) {
    if (geteuid() != 0) {
        printf("ping: sudo rights are required, exiting.\n");
        return 1;
    }
    struct ping_context ctx = {0};
    init(&ctx);
    install_handlers();
    disable_echoctl();
    parse_flags(&ctx, argc, argv);
    struct addrinfo hints = {0};
    hints.ai_family = AF_INET;

    create_socket(ctx.options, &ctx);
    int err = getaddrinfo(ctx.res.arg_address, NULL, &hints, &ctx.destination_addrinfo);
    if (ctx.options.verbose)
        print_addr_verbose(&ctx);
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
    while (state.running) {
        if (state.sending) {
            send_packet(&packet, &ctx);
            if (ctx.interval_s != INTERVAL_FLOOD_S)
                state.sending = false;
            alarm(ctx.interval_s);
        }
        handle_reply(&ctx);
        handle_state(&state, ctx);
    }
    print_stats(ctx.res, ctx.options.payload_size);
    cleanup(&ctx);
    return 0;
}

static void handle_sig(int sig) {
    if (sig == SIGQUIT) {
        state.printing = true;
    } else if (sig == SIGINT) {
        state.running = false;
    } else if (sig == SIGALRM) {
        state.sending = true;
    }
    return;
}

static void install_handlers(void) {
    struct sigaction sa = {0};
    sa.sa_handler = handle_sig;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGALRM, &sa, NULL);
    sigaction(SIGQUIT, &sa, NULL);
}

static void init(struct ping_context *ctx) {
    ctx->options.payload_size = PAYLOAD_SIZE;
    ctx->res.rrt_in.min_time = UINT32_MAX;
    ctx->seq = 1;
    ctx->interval_s = INTERVAL_S;

    state.running = true;
    state.printing = false;
    state.sending = true;
}