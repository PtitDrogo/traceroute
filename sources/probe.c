#include "../includes/traceroute.h"

void init_probes(probe_record probes) { (void)probes; }

struct probe_index get_probe_index_from_sequence(uint16_t seq) {
    struct probe_index idx = {0};
    idx.ttl = seq / 10;
    idx.probe = seq % 10;
    return idx;
}

struct timespec get_probe_time(probe_record probes[MAX_TTL][MAX_PROBES], uint16_t seq) {
    struct probe_index idx = get_probe_index_from_sequence(seq);
    return probes[idx.ttl][idx.probe].sent_at;
}

// Return the index of the first next instance of pending probe
// if the probe you give it is the oldest, it will just return that.
// Increases count by one for every row walked.
struct probe_index find_next_oldest_probe_index(struct probe_index cur, probe_record probes[MAX_TTL][MAX_PROBES],
                                                uint8_t *count) {
    struct probe_index res;

    for (uint8_t ttl = cur.ttl; ttl < MAX_TTL; ttl++) {
        for (uint8_t probe = cur.probe; probe < MAX_PROBES; probe++) {
            if (probes[ttl][probe].status == PENDING) {
                res.ttl = ttl;
                res.probe = probe;
                return res;
            }
            *count += 1;
        }
        cur.probe = 0;
    }
    res.probe = MAX_PROBES;
    res.ttl = MAX_TTL;
    return res;
};


uint8_t handle_responded_probes(struct ping_context *ctx, struct probe_index *oldest_i) {
    uint8_t responded_probes = 0;
    struct probe_index idx = find_next_oldest_probe_index(*oldest_i, ctx->probes, &responded_probes);

    oldest_i->probe = idx.probe;
    oldest_i->ttl = idx.ttl;
    probe_record *oldest_probe = &ctx->probes[idx.ttl][idx.probe];

    double time_since_sent_ms = compute_time_difference(oldest_probe->sent_at);

    if (time_since_sent_ms <= 1000)
        return responded_probes;

    // If we are here, our probe is in PENDING and is past the timeout.
    // So we change its status to Timeout and all the other vars.
    oldest_probe->status = TIMEOUT;
    oldest_probe->time_rtt = -1;
    strlcpy(oldest_probe->resolved_address, "* ", sizeof("* "));

    oldest_i->probe += 1;
    if (oldest_i->probe == MAX_PROBES) {
        oldest_i->probe = 0;
        oldest_i->ttl += 1;
    }
    return responded_probes + 1; // in theory were never here
}
