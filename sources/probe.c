#include "../includes/traceroute.h"

void init_probes(struct probe_record probes) { (void)probes; }

struct probe_index get_probe_index_from_sequence(uint16_t seq) {
    struct probe_index idx = {0};
    idx.ttl = seq / 10;
    idx.probe = seq % 10;
    return idx;
}

struct timespec get_probe_time(struct probe_record probes[MAX_TTL][MAX_PROBES], uint16_t seq) {
    struct probe_index idx = get_probe_index_from_sequence(seq);
    return probes[idx.ttl][idx.probe].sent_at;
}


//REWRITE THIS SHIT UNTIL YOU HAVE SOMETHING THAT MAKES SENSE
uint8_t timeout_outdated_probes(struct ping_context *ctx, struct probe_index *oldest_i) {
    struct probe_record oldest_probe = ctx->probes[oldest_i->ttl][oldest_i->probe];
    double time_since_sent_ms = compute_time_difference(oldest_probe.sent_at);

    if (time_since_sent_ms <= 1000)
        return 0;

    uint8_t timeout_probes = 1;

    // print the one confirmed timeout probe.
    if (oldest_probe.status != RESPONDED) {
        oldest_probe.status = TIMEOUT;
        printf("* ");
        if (oldest_i->probe == MAX_PROBES - 1)
            printf("\n");
        if (oldest_i->ttl == MAX_TTL - 1 && oldest_i->probe == MAX_PROBES - 1) {
            cleanup(ctx);
            exit(EXIT_SUCCESS);
        }
    }

    oldest_i->probe += 1;
    if (oldest_i->probe == MAX_PROBES) {
        oldest_i->probe = 0;
        oldest_i->ttl += 1;
    }

    for (uint8_t ttl = oldest_i->ttl; ttl < MAX_TTL; ttl++) {
        for (uint8_t probe = oldest_i->probe; probe < MAX_PROBES; probe++) {
            // if (ctx->probes[ttl][probe].status == RESPONDED) {
            //     timeout_probes += 1;
            //     continue;
            // }
            if (ctx->probes[ttl][probe].status == PENDING) {
                oldest_i->ttl = ttl;
                oldest_i->probe = probe;
                return timeout_probes;
            }
            timeout_probes += 1;
            ctx->probes[ttl][probe].status = TIMEOUT;
            printf("* ");
            if (probe == MAX_PROBES - 1)
                printf("\n");
            if (ttl == MAX_TTL - 1 && probe == MAX_PROBES - 1) {
                cleanup(ctx);
                exit(EXIT_SUCCESS);
            }
        }
        oldest_i->probe = 0;
    }
    printf("traceroute: impossible probe status\n");
    return -1; // in theory were never here
}