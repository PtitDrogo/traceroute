#include "../includes/traceroute.h"

void init_probes(probe_record_t probes) { (void)probes; }

probe_index_t get_probe_index_from_sequence(uint16_t seq) {
    // uint16_t seq = ntohs(icmp->header.un.echo.sequence);
    probe_index_t idx = {0};
    idx.ttl = seq / 10;
    idx.probe = seq % 10;
    return idx;
}

probe_index_t get_probe_index_from_port(uint16_t port) {
    probe_index_t idx = {0};

    idx.ttl = (port - BASE_UDP_PORT) / 10;
    idx.probe = (port - BASE_UDP_PORT) % 10;
    return idx;
}

struct timespec get_probe_time(probe_record_t probes[MAX_TTL][MAX_PROBES], uint16_t seq) {
    probe_index_t idx = get_probe_index_from_sequence(seq);
    return probes[idx.ttl][idx.probe].sent_at;
}

// Return the index of the first next instance of pending probe
// if the probe you give it is the oldest, it will just return that.
// Increases count by one for every row walked.
probe_index_t find_next_oldest_probe_index(probe_index_t cur, probe_record_t probes[MAX_TTL][MAX_PROBES],
                                           uint8_t *count, uint8_t max_ttl, uint8_t max_probes_per_ttl) {
    probe_index_t res;

    for (uint8_t ttl = cur.ttl; ttl < max_ttl; ttl++) {
        for (uint8_t probe = cur.probe; probe < max_probes_per_ttl; probe++) {
            if (probes[ttl][probe].status == PENDING) {
                res.ttl = ttl;
                res.probe = probe;
                return res;
            }
            *count += 1;
        }
        cur.probe = 0;
    }
    res.probe = max_probes_per_ttl;
    res.ttl = max_ttl;
    return res;
};

void print_probe_struct(ping_context_t *ctx) {
    static const char *status_names[] = {"PENDING", "RESPONDED", "TIMEOUT"};
    for (uint8_t ttl = 0; ttl <= ctx->final_ttl_index; ttl++) {
        for (uint8_t probe = 0; probe < ctx->options.max_probes_per_ttl; probe++) {
            printf("probes: [%d][%d] = %s\n", ttl, probe, status_names[ctx->probes[ttl][probe].status]);
            printf("addr: %s, sent_at: %ld, time_rtt: %f\n", ctx->probes[ttl][probe].resolved_address,
                   ctx->probes[ttl][probe].sent_at.tv_sec, ctx->probes[ttl][probe].time_rtt);
        }
    }
}

uint8_t handle_responded_probes(ping_context_t *ctx, probe_index_t *oldest_i) {
    uint8_t responded_probes = 0;
    probe_index_t idx = find_next_oldest_probe_index(*oldest_i, ctx->probes, &responded_probes, ctx->final_ttl_index,
                                                     ctx->options.max_probes_per_ttl);
    if (responded_probes != 0) {
        printf("Responded probe isnt 0 ! -> %d\n", responded_probes);
        printf("oldest probe -> ttl : %d, probe_i : %d\n", idx.ttl, idx.probe);
        
    }
    oldest_i->probe = idx.probe;
    oldest_i->ttl = idx.ttl;
    probe_record_t *oldest_probe = &ctx->probes[idx.ttl][idx.probe];

    double time_since_sent_ms = compute_time_difference(oldest_probe->sent_at);

    if (time_since_sent_ms <= 3000)
        return responded_probes;
    // If we are here, our probe is in PENDING and is past the timeout.
    // So we change its status to Timeout and all the other vars.
    oldest_probe->status = TIMEOUT;
    oldest_probe->time_rtt = -1;
    ft_strlcpy(oldest_probe->resolved_address, "* ", sizeof("* "));

    oldest_i->probe += 1;
    if (oldest_i->probe == ctx->options.max_probes_per_ttl) {
        oldest_i->probe = 0;
        oldest_i->ttl += 1;
    }
    printf("Oldest probe is increased by 1 because the current one is a timeout\n");
    return responded_probes + 1; // in theory were never here
}
