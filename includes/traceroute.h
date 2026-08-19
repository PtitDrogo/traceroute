#ifndef FT_TRACEROUTE
#define FT_TRACEROUTE

#include <arpa/inet.h>
#include <stdio.h>

#include <math.h>
#include <netdb.h>
#include <netinet/ip_icmp.h>
#include <netinet/udp.h>
#include <poll.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>

#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

#define BASE_UDP_PORT 33434
#define PAYLOAD_SIZE 56
#define DEFAULT_TTL 30
#define DEFAULT_PROBE 3
#define MAX_TTL 255
#define MAX_PROBES 10
#define DEFAULT_IN_FLIGHT_PROBES 16
#define TIMEOUT_TIME_MS 1000
#define SLEEP_TIME_MS 10

#define HELP_STRING                                                                                                    \
    "Usage\n"                                                                                                          \
    "ft_traceroute traceroute [ -InrV ] [ -m max_ttl ] [ -N squeries ] [ -q nqueries ]  [ -z sendwait ]  host\n"       \
    "Options:\n"                                                                                                       \
    "  -I                   Use ICMP ECHO for tracerouting\n"                                                          \
    "  -m max_ttl\n"                                                                                                   \
    "           Set the max number of hops (max TTL to be\n"                                                           \
    "           reached). Default is 30\n"                                                                             \
    "  -n       Do not resolve IP addresses to their domain names\n"                                                   \
    "  -q nqueries\n"                                                                                                  \
    "           Set the number of probes per each hop. Default is\n"                                                   \
    "           3\n"                                                                                                   \
    "  -z sendwait\n"                                                                                                  \
    "           Minimal time interval between probes (default 10 ms).\n"                                               \
    "           If the value is more than 10, then it specifies a\n"                                                   \
    "           number in milliseconds, else it is a number of\n"                                                      \
    "           seconds (float point values allowed too)\n"                                                            \
    "  -r       Bypass the normal routing and send directly to a\n"                                                    \
    "           host on an attached network\n"                                                                         \
    "  -V       print version and exit\n"                                                                              \
    "Arguments:\n"                                                                                                     \
    "+     host          The host to traceroute to\n"

typedef struct {
    struct icmphdr header;
    char payload[UINT16_MAX];
} icmp_packet_t;

typedef struct {
    char resolved_address[NI_MAXHOST];
} response_data_t;

typedef enum { NOT_SENT, PENDING, RESPONDED, TIMEOUT } probe_status;

typedef struct {
    char ip[INET_ADDRSTRLEN];
    struct timespec sent_at;
    probe_status status;
    double time_slept_when_sent_ms;
    double time_lost_by_dns_when_sent_ms;
    double time_rtt;

} probe_record_t;

typedef struct {
    uint8_t max_probes_per_ttl;
    uint32_t max_probes_in_flight;
    double time_to_sleep_ms;
    bool use_icmp;
    bool skip_dns;
    bool skip_routing;
} options_t;

typedef struct {
    uint8_t ttl;
    uint8_t probe;
} probe_index_t;

typedef struct {
    probe_record_t probes[MAX_TTL][MAX_PROBES];
    uint8_t curr_ttl_to_print;
    uint8_t final_ttl_index;
    response_data_t res;
    options_t options;
    // Basic start data
    char *arg_address;
    struct addrinfo *destination_addrinfo;
    int send_sock;
    int recv_sock;
    double time_lost_by_dns;
    double time_slept_ms;
} tr_context_t;

// send
icmp_packet_t build_icmp_packet();
void create_socket(tr_context_t *ctx);
void update_socket(tr_context_t *ctx, uint8_t ttl);
uint16_t checksum(const uint16_t *data, size_t size);

void update_packet(void *packet, probe_index_t index, tr_context_t *ctx);
void send_packet(void *packet, tr_context_t *ctx);
void send_protocol(uint8_t send_i, tr_context_t *ctx, void *packet);

// reply
void handle_reply(tr_context_t *ctx);

// helpers
void cleanup(tr_context_t *ctx);
void get_hostname_string_from_ip(const char *ip_str, char *dest_buf, size_t buf_len);
size_t ft_strlcpy(char *dst, const char *src, size_t dsize);

// parsing
void parse_flags(tr_context_t *ctx, int argc, char *argv[]);
long parse_num(tr_context_t *ctx, uint32_t max_range, char opt_char);
double parse_float(tr_context_t *ctx, uint32_t max_range, char opt_char);

// print
void print_start_string(const tr_context_t *ctx);
void print_ready_ttl_groups(tr_context_t *ctx);

// probe
uint8_t handle_responded_probes(tr_context_t *ctx, probe_index_t *oldest_i);
probe_index_t get_decoded_probe_index_from_seq(uint16_t encoded);

// timers
double compute_time_difference(const struct timespec past_time);
double sleep_and_measure(double time_to_sleep_ms);

#endif