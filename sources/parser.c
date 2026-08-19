#include "../includes/traceroute.h"

void parse_flags(tr_context_t *ctx, int argc, char *argv[]) {
    int32_t opt;
    const char *optstring = ":hVm:q:IN:z:nr";

    while ((opt = getopt(argc, argv, optstring)) != -1) {
        switch (opt) {
        case 'r':
            ctx->options.skip_routing = true;
            break;
        case 'n':
            ctx->options.skip_dns = true;
            break;
        case 'I':
            ctx->options.use_icmp = true;
            break;
        case 'N':
            ctx->options.max_probes_in_flight = (uint32_t)parse_num(ctx, INT32_MAX, 'N');
            break;
        case 'z':
            ctx->options.time_to_sleep_ms = parse_float(ctx, INT32_MAX, 'z');
            if (ctx->options.time_to_sleep_ms <= 10) {
                ctx->options.time_to_sleep_ms *= 1000;
            }
            break;
        case 'm':
            ctx->final_ttl_index = (uint8_t)parse_num(ctx, UINT8_MAX, 'm') - 1;
            break;
        case 'q':
            ctx->options.max_probes_per_ttl = (uint8_t)parse_num(ctx, 10, 'q');
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
    uint32_t max_theorical_probes = (ctx->final_ttl_index + 1) * ctx->options.max_probes_per_ttl;
    ctx->options.max_probes_in_flight =
        (uint32_t)fmin(ctx->options.max_probes_in_flight, fmin(max_theorical_probes, 255));
    return;
}

long parse_num(tr_context_t *ctx, uint32_t max_range, char opt_char) {
    char *endptr;
    long val = strtod(optarg, &endptr);
    if (*endptr != '\0' || optarg == endptr) {
        fprintf(stderr, "traceroute: Invalid arguments '%s'\n", optarg);
        cleanup(ctx);
        exit(1);
    }
    if (val <= 0 || val > max_range) {
        fprintf(stderr, "traceroute: invalid -%c value: '%s': out of range: 0 <= value <= %d\n", opt_char, optarg,
                max_range);
        cleanup(ctx);
        exit(1);
    }
    return val;
}

double parse_float(tr_context_t *ctx, uint32_t max_range, char opt_char) {
    char *endptr;
    double val = strtod(optarg, &endptr);
    if (*endptr != '\0' || optarg == endptr) {
        fprintf(stderr, "traceroute: Invalid arguments '%s'\n", optarg);
        cleanup(ctx);
        exit(1);
    }
    if (val <= 0 || val > max_range) {
        fprintf(stderr, "traceroute: invalid -%c value: '%s': out of range: 0 <= value <= %d\n", opt_char, optarg,
                max_range);
        cleanup(ctx);
        exit(1);
    }
    return val;
}
