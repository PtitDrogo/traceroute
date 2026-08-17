#include "../includes/traceroute.h"

//Only use for ICMP :)
probe_index_t get_decoded_probe_index_from_seq(uint16_t encoded) {
    probe_index_t idx = {0};
    idx.ttl = (encoded) / 10;
    idx.probe = (encoded) % 10;
    return idx;
}


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
        if (p->status == NOT_SENT)
            break;
        if (p->status == PENDING) {
            if (compute_time_difference(p->sent_at) <= TIMEOUT_TIME_MS)
                break;
            p->status = TIMEOUT;
            p->time_rtt = -1;
            ft_strlcpy(p->ip, "TIMEOUT", sizeof(p->ip));
        }
        freed++;
        idx.probe++;
        if (idx.probe == ctx->options.max_probes_per_ttl) {
            idx.probe = 0;
            idx.ttl++;
        }
    }
    *oldest_i = idx;
    return freed;
}
