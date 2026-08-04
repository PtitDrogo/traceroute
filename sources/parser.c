#include "../includes/traceroute.h"

void parse_flags(struct ping_context *ctx, int argc, char *argv[]) {
    int32_t opt;
    const char *optstring = ":hVm:";

    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
        case 'm':
            ctx->options.max_ttl = (uint8_t)parse_num(ctx, UINT8_MAX, 'm');
            break;
        case 'h':
            printf(HELP_STRING);
            exit(0);
        case 'V':
            printf("ft_traceroute from tfreydie.\n");
            exit(0);
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
        fprintf(stderr, "traceroute: usage error: Destination address required\n");
        exit(1);
    } else if (remaining > 1) {
        fprintf(stderr, "traceroute: usage error: Too many destination addresses provided\n");
        exit(1);
    }

    ctx->arg_address = argv[optind];
    return;
}

long parse_num(struct ping_context *ctx, uint32_t max_range, char opt_char) {
    char *endptr;
    long val = strtol(optarg, &endptr, 10);
    if (*endptr != '\0' || optarg == endptr) {
        fprintf(stderr, "traceroute: Invalid arguments '%s'\n", optarg);
        cleanup(ctx);
        exit(1);
    }
    if (val < 0 || val > max_range) {
        fprintf(stderr, "traceroute: invalid -%c value: '%s': out of range: 0 <= value <= %d\n", opt_char, optarg,
                max_range);
        cleanup(ctx);
        exit(1);
    }
    return val;
}
