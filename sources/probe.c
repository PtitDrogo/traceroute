#include "../includes/traceroute.h"

// struct probe_record {
//     struct timespec sent_at;
//     int ttl;
//     bool replied;
// };

// since 10 is the max, but that will be a variable later

// when I receive a seq number, I want to be able to encode info with it.
// sequence = ttl 10 3 + probe_idx

/*
int ttl       = sequence / 10;
int probe_idx = sequence % 10;
struct timespec sent_at = probes[ttl][probe_idx].sent_at;

*/

void init_probes(struct probe_record probes) { (void)probes; }

struct timespec get_probe_time(struct probe_record probes[MAX_TTL][MAX_PROBES], uint16_t seq) {
    int ttl = seq / 10;
    int probe_idx = seq % 10;
    return probes[ttl][probe_idx].sent_at;
}