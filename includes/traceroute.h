#ifndef FT_PING
#define FT_PING

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
    "ft_traceroute traceroute [ -I ] [ -f first_ttl ]  [ -m max_ttl ] [ -N squeries ] [ -q nqueries ]  [ -z "          \
    "sendwait ]  host \n"                                                                                              \
    "Options:\n"                                                                                                       \
    "  -f first_ttl\n"                                                                                                 \
    "           Start from the first_ttl hop(instead of 1)\n"                                                          \
    "  -I                   Use ICMP ECHO for tracerouting\n"                                                          \
    "  -m max_ttl\n"                                                                                                   \
    "           Set the max number of hops (max TTL to be\n"                                                           \
    "           reached). Default is 30\n"                                                                             \
    "  -q nqueries\n"                                                                                                  \
    "           Set the max number of hops (max TTL to be\n"                                                           \
    "           reached). Default is 30\n"                                                                             \
    "  -z sendwait\n"                                                                                                  \
    "           Minimal time interval between probes (default 10 ms).\n"                                               \
    "           If the value is more than 10, then it specifies a\n"                                                   \
    "           number in milliseconds, else it is a number of\n"                                                      \
    "           seconds (float point values allowed too)\n"                                                            \
    "  -V                   print version and exit\n"                                                                  \
    "Arguments:\n"                                                                                                     \
    "+     host          The host to traceroute to\n"

// Contains the icmp header and the payload
typedef struct {
    struct icmphdr header;
    char payload[UINT16_MAX];
} icmp_packet_t;

typedef struct {
    struct udphdr header;
    char payload[UINT16_MAX];
} udp_packet_t;

typedef struct {
    char resolved_address[NI_MAXHOST];
} response_data_t;

typedef enum { NOT_SENT, PENDING, RESPONDED, TIMEOUT } probe_status;

typedef struct {
    struct timespec sent_at;
    double time_slept_when_sent_ms;
    double time_lost_by_dns_when_sent_ms;
    probe_status status;
    // char resolved_address[NI_MAXHOST]; //in theory I dont need to store this ? I can just compute it when I want to
    // print it, that saves a LOT of memory too :)
    double time_rtt;
    char ip[INET_ADDRSTRLEN];

} probe_record_t;

typedef struct {
    uint8_t max_probes_per_ttl;
    uint32_t max_probes_in_flight;
    bool use_icmp;
    double time_to_sleep_ms;
    uint8_t first_ttl;
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
} ping_context_t;

// send
icmp_packet_t build_icmp_packet();
void create_socket(ping_context_t *ctx);
void update_socket(ping_context_t *ctx, uint8_t ttl);
uint16_t checksum(const uint16_t *data, size_t size);

void update_packet(void *packet, probe_index_t index, ping_context_t *ctx);
void send_packet(void *packet, ping_context_t *ctx);
void send_protocol(uint8_t send_i, ping_context_t *ctx, void *packet);

// reply
void handle_reply(ping_context_t *ctx);

// helpers
void cleanup(ping_context_t *ctx);
void get_hostname_string_from_ip(const char *ip_str, char *dest_buf, size_t buf_len);
size_t ft_strlcpy(char *dst, const char *src, size_t dsize);

// parsing
void parse_flags(ping_context_t *ctx, int argc, char *argv[]);
long parse_num(ping_context_t *ctx, uint32_t max_range, char opt_char);
double parse_float(ping_context_t *ctx, uint32_t max_range, char opt_char);

// Getting rid the \\ printing
void disable_echoctl(void);
void restore_termios(void);

// print
void print_start_string(const ping_context_t *ctx);
void print_ready_ttl_groups(ping_context_t *ctx);

// probe
uint8_t handle_responded_probes(ping_context_t *ctx, probe_index_t *oldest_i);
probe_index_t get_decoded_probe_index_from_seq(uint16_t encoded);
void print_probe_struct(ping_context_t *ctx);

// timers
double compute_time_difference(const struct timespec past_time);
double sleep_and_measure(double time_to_sleep_ms);

#endif