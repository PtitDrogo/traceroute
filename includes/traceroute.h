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

// Since ICMP Header is 8 bytes, 56 is standard so that its 64 total
#define BASE_UDP_PORT 33434
#define PAYLOAD_SIZE 56
#define DEFAULT_TTL 30
#define DEFAULT_PROBE 3
#define MAX_TTL 100 //If youre human youll probably just heap allocate this later using the param given by the user
#define MAX_PROBES 10
#define DEFAULT_IN_FLIGHT_PROBES 16

#define HELP_STRING                                                                                                    \
    "Usage\n"                                                                                                          \
    "  ft_route [options] <destination>\n"                                                                             \
    "Options:\n"                                                                                                       \
    "  <destination>      DNS name or IP address\n"                                                                    \
    "  -V                 print version and exit\n"

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

typedef enum { PENDING, RESPONDED, TIMEOUT } probe_status;

typedef struct {
    struct timespec sent_at;
    probe_status status;
    char resolved_address[NI_MAXHOST];
    double time_rtt;
    char ip[INET_ADDRSTRLEN];
} probe_record_t;

typedef struct {
    uint8_t max_probes_per_ttl;
    uint32_t max_probes_in_flight;
    bool use_icmp;
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
double compute_time_difference(const struct timespec past_time);
size_t ft_strlcpy(char *dst, const char *src, size_t dsize);

// parsing
void parse_flags(ping_context_t *ctx, int argc, char *argv[]);
long parse_num(ping_context_t *ctx, uint32_t max_range, char opt_char);

// Getting rid the \\ printing
void disable_echoctl(void);
void restore_termios(void);

// print
void print_start_string(const ping_context_t *ctx);
void print_ready_ttl_groups(ping_context_t *ctx);

// probe
uint8_t handle_responded_probes(ping_context_t *ctx, probe_index_t *oldest_i);
probe_index_t get_decoded_probe_index(uint16_t encoded, bool use_icmp);
void print_probe_struct(ping_context_t *ctx);

#endif