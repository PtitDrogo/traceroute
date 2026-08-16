#include "../includes/traceroute.h"

probe_index_t get_decoded_probe_index(uint16_t encoded, bool use_icmp) {
    probe_index_t idx = {0};
    idx.ttl = (encoded - (BASE_UDP_PORT * !use_icmp)) / 10;
    idx.probe = (encoded - (BASE_UDP_PORT * !use_icmp)) % 10;
    return idx;
}

// Return the index of the first next instance of pending probe
// if the probe you give it is the oldest, it will just return that.
// Increases count by one for every row walked.
probe_index_t find_next_oldest_probe_index(probe_index_t cur, probe_record_t probes[MAX_TTL][MAX_PROBES],
                                           uint8_t *count, uint8_t max_ttl_index, uint8_t max_probes_per_ttl) {
    probe_index_t res;

    for (uint8_t ttl = cur.ttl; ttl <= max_ttl_index; ttl++) {
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
    res.ttl = max_ttl_index;
    return res;
};

void print_probe_struct(ping_context_t *ctx) {
    static const char *status_names[] = {"PENDING", "RESPONDED", "TIMEOUT"};
    for (uint8_t ttl = 0; ttl <= ctx->final_ttl_index; ttl++) {
        for (uint8_t probe = 0; probe < ctx->options.max_probes_per_ttl; probe++) {
            printf("probes: [%d][%d] = %s\n", ttl, probe, status_names[ctx->probes[ttl][probe].status]);
            printf("ip: %s, sent_at: %ld, time_rtt: %f\n", ctx->probes[ttl][probe].ip,
                   ctx->probes[ttl][probe].sent_at.tv_sec, ctx->probes[ttl][probe].time_rtt);
        }
    }
}

uint8_t handle_responded_probes(ping_context_t *ctx, probe_index_t *oldest_i) {
    uint8_t freed = 0;
    probe_index_t idx = *oldest_i;

    while (idx.ttl <= ctx->final_ttl_index) {
        probe_record_t *p = &ctx->probes[idx.ttl][idx.probe];
        if (p->status == PENDING) {
            if (compute_time_difference(p->sent_at) <= 3000)
                break; 
            p->status = TIMEOUT;
            p->time_rtt = -1;
            ft_strlcpy(p->ip, "TIMEOUT", sizeof(p->ip));
        }
        freed++;
        idx.probe++;
        if (idx.probe == ctx->options.max_probes_per_ttl) { idx.probe = 0; idx.ttl++; }
    }
    *oldest_i = idx;
    return freed;
}
