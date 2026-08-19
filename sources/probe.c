#include "../includes/traceroute.h"

probe_index_t get_decoded_probe_index_from_seq(uint16_t encoded) {
    probe_index_t idx = {0};
    idx.ttl = (encoded) / MAX_PROBES;
    idx.probe = (encoded) % MAX_PROBES;
    return idx;
}

uint8_t handle_responded_probes(tr_context_t *ctx, probe_index_t *oldest_i) {
    uint8_t freed = 0;
    probe_index_t idx = *oldest_i;

    while (idx.ttl <= ctx->final_ttl_index) {
        probe_record_t *p = &ctx->probes[idx.ttl][idx.probe];
        if (p->status == NOT_SENT)
            break;
        if (p->status == PENDING) {
            double time_slept_ms = ctx->time_slept_ms - p->time_slept_when_sent_ms;
            double time_lost_by_dns_ms = ctx->time_lost_by_dns - p->time_lost_by_dns_when_sent_ms;

            if (compute_time_difference(p->sent_at) <= TIMEOUT_TIME_MS + time_slept_ms + (time_lost_by_dns_ms / 2))
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
