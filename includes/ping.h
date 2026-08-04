#ifndef FT_PING
#define FT_PING

#include <arpa/inet.h>
#include <assert.h>
#include <netinet/in.h>
#include <stdio.h>

#include <netdb.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include <netinet/ip_icmp.h>

#include <errno.h>
#include <math.h>
#include <signal.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <termios.h>

// Since ICMP Header is 8 bytes, 56 is standard so that its 64 total
#define PAYLOAD_SIZE 56
#define ALPHA 0.125
#define INTERVAL_S 1
#define INTERVAL_FLOOD_S 0.01
#define MAX_PAYLOAD_SIZE 65507

#define HELP_STRING                                                                                                    \
    "Usage\n"                                                                                                          \
    "  ft_ping [options] <destination>\n"                                                                              \
    "Options:\n"                                                                                                       \
    "  <destination>      DNS name or IP address\n"                                                                    \
    "  -f                 flood ping\n"                                                                                \
    "  -c <count>         stop after <count> replies\n"                                                                \
    "  -n                 no reverse DNS name resolution\n"                                                            \
    "  -p <pattern>       contents of padding byte\n"                                                                  \
    "  -r                 ignore normal routing tables and send directly to a host on an attached "                    \
    "interface\n"                                                                                                      \
    "  -s <size>          use <size> as number of data bytes to be sent\n"                                             \
    "  -v                 verbose output\n"                                                                            \
    "  -V                 print version and exit\n"                                                                    \
    "  -w <deadline>      reply wait <deadline> in seconds\n"                                                          \
    "  -h                 print help and exit\n"

// Contains the icmp header and the payload
struct ping_packet {
    struct icmphdr header;
    char payload[UINT16_MAX];
};

// rtt = round Time trip
struct rtt_in {
    double min_time;
    double max_time;
    double sum_rtt;
    double sum_rtt_squared;
    double ewma; // EWMA = Exponentially Weighted Moving Average It values recent responses more than old ones.
};

struct ping_result {
    uint32_t packets_transmitted;
    uint32_t packets_received;
    struct timespec start_time;
    struct rtt_in rrt_in;

    // These too can really easily be moved away from the struct tbh.
    char *arg_address;
    char resolved_address[NI_MAXHOST];
};

struct pattern {
    uint8_t buf[33];
    uint8_t len;
};

struct options {
    uint16_t payload_size;
    bool no_dns;
    bool verbose;
    bool dont_route;
    uint32_t timeout_total_ms;
    uint32_t max_send;
    uint8_t ttl;
    struct pattern pattern;
};

struct ping_context {
    struct addrinfo *destination_addrinfo;
    struct ping_result res;
    struct options options;
    int sock;
    double interval_s;
    uint16_t seq;
};

struct state {
    volatile sig_atomic_t running;
    volatile sig_atomic_t printing;
    volatile sig_atomic_t sending;
};

// send
void send_packet(struct ping_packet *packet, struct ping_context *ctx);
void build_packet(struct ping_packet *packet, struct ping_context *ctx);
void update_packet(struct ping_packet *packet, struct ping_context *ctx);
void create_socket(struct options options, struct ping_context *ctx);
uint16_t checksum(const uint16_t *data, size_t size);

// reply
void update_ping_info(double time_rtt, struct ping_context *ctx);
void handle_reply(struct ping_context *ctx);
void handle_state(struct state *state, struct ping_context ctx);

// helpers
void cleanup(struct ping_context *ctx);
int32_t get_hostname_string_from_ip(const char *ip_str, char *dest_buf, size_t buf_len, bool no_dns);
double compute_time_difference(const struct timespec past_time);

// parsing
void parse_flags(struct ping_context *ctx, int argc, char *argv[]);
long parse_num(struct ping_context *ctx, uint32_t max_range);
void handle_pattern(struct ping_context *ctx);

// Getting rid the \\ printing
void disable_echoctl(void);
void restore_termios(void);

// printing
void print_stats(struct ping_result res, uint16_t payload_size);
void print_curr_stats(struct ping_result res, uint16_t payload_size);
void print_socket_verbose(const struct ping_context *ctx);
void print_addr_verbose(const struct ping_context *ctx);

// debug
void print_pattern_reply(struct ping_context *ctx, void *payload);

#endif