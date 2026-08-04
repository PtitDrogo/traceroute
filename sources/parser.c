#include "../includes/ping.h"

void parse_flags(struct ping_context *ctx, int argc, char *argv[]) {
    int32_t opt;
    const char *optstring = ":fc:nw:W:p:rs:vVht:";

    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
        case 't':
            ctx->options.ttl = (uint8_t)parse_num(ctx, 255); // TTL max is 255 (1 byte field)
            if (!ctx->options.ttl) {
                printf("ping: cannot set unicast time-to-live: Invalid argument\n");
                exit(1);
            }
            break;
        case 'h':
            printf(HELP_STRING);
            exit(0);
        case 'V':
            printf("ft_ping from tfreydie.\n");
            exit(0);
        case 'v':
            ctx->options.verbose = true;
            break;
        case 'f':
            ctx->interval_s = INTERVAL_FLOOD_S;
            break;
        case 'c':
            ctx->options.max_send = (uint32_t)parse_num(ctx, INT32_MAX);
            break;
        case 'n':
            ctx->options.no_dns = true;
            break;
        case 'w':
            ctx->options.timeout_total_ms = (uint32_t)parse_num(ctx, INT32_MAX) * 1000;
            break;
        case 'p':
            handle_pattern(ctx);
            break;
        case 'r':
            ctx->options.dont_route = true;
            break;
        case 's':
            ctx->options.payload_size = (uint16_t)parse_num(ctx, MAX_PAYLOAD_SIZE);
            break;
        case ':': // Missing required argument
            fprintf(stderr, "ping: option requires an argument --%c\n", optopt);
            printf(HELP_STRING);
            exit(1);
            break;
        case '?':
            printf("ping: invalid option -- '%c'\n", optopt);
            printf(HELP_STRING);
            exit(1);
            break;
        default:
            break;
        }
    }

    int remaining = argc - optind;

    if (remaining < 1) {
        fprintf(stderr, "ping: usage error: Destination address required\n");
        exit(1);
    } else if (remaining > 1) {
        fprintf(stderr, "ping: usage error: Too many destination addresses provided\n");
        exit(1);
    }

    ctx->res.arg_address = argv[optind];
    return;
}

long parse_num(struct ping_context *ctx, uint32_t max_range) {
    char *endptr;
    long val = strtol(optarg, &endptr, 10);
    if (*endptr != '\0' || optarg == endptr) {
        fprintf(stderr, "ping: Invalid arguments '%s'\n", optarg);
        cleanup(ctx);
        exit(1);
    }
    if (val < 0 || val > max_range) {
        fprintf(stderr, "ping: invalid -s value: '%s': out of range: 0 <= value <= %d\n", optarg, max_range);
        cleanup(ctx);
        exit(1);
    }
    return val;
}

void handle_pattern(struct ping_context *ctx) {
    size_t len = strlen(optarg);
    size_t pattern_len = (len + 1) / 2; // ceil(len / 2)
    if (pattern_len > 16) {
        pattern_len = 16;
    }

    size_t i;
    for (i = 0; i < len / 2 && i < pattern_len; i++) {
        unsigned int byte;
        if (sscanf(optarg + i * 2, "%2x", &byte) != 1) {
            fprintf(stderr, "ping: patterns must be specified as hex digits: %s\n", optarg);
            cleanup(ctx);
            exit(1);
        }
        ctx->options.pattern.buf[i] = (uint8_t)byte;
    }

    // handle trailing char
    if (len % 2 != 0 && i < pattern_len) {
        unsigned int byte;
        if (sscanf(optarg + i * 2, "%1x", &byte) != 1) {
            fprintf(stderr, "ping: patterns must be specified as hex digits: %s\n", optarg);
            cleanup(ctx);
            exit(1);
        }
        ctx->options.pattern.buf[i] = (uint8_t)byte;
    }

    ctx->options.pattern.len = pattern_len;
    printf("PATTERN: 0x");
    for (size_t i = 0; i < ctx->options.pattern.len; i++) {
        printf("%02x", ctx->options.pattern.buf[i]);
    }
    printf("\n");
    return;
}
