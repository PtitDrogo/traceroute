#ifndef FT_PING
#define FT_PING

#include <arpa/inet.h>
#include <stdio.h>

#include <netdb.h>
#include <netinet/ip_icmp.h>
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
#define PAYLOAD_SIZE 56
#define DEFAULT_TTL 30
#define MAX_TTL UINT8_MAX
#define MAX_PROBES 3

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
} ping_packet_t;

typedef struct {
    char resolved_address[NI_MAXHOST];
} response_data_t;

typedef enum { PENDING, RESPONDED, TIMEOUT } probe_status;

typedef struct {
    struct timespec sent_at;
    probe_status status;
    char resolved_address[NI_MAXHOST];
    double time_rtt;
} probe_record_t;

typedef struct {
    uint8_t max_probes;
} options_t;

typedef struct {
    uint8_t ttl;
    uint8_t probe;
} probe_index_t;

typedef struct {
    probe_record_t probes[MAX_TTL][MAX_PROBES];
    uint8_t curr_ttl_to_print;
    uint8_t final_ttl; // This has to be INIT at MAX_TTL, then only lowered.
    response_data_t res;
    options_t options;
    // Basic start data
    char *arg_address;
    struct addrinfo *destination_addrinfo;
    int sock;
} ping_context_t;

// send
void send_packet(ping_packet_t *packet, ping_context_t *ctx);
void build_packet(ping_packet_t *packet);
void update_packet(ping_packet_t *packet, uint8_t ttl, uint8_t probe_index);
void create_socket(options_t options, ping_context_t *ctx);
void update_socket(ping_context_t *ctx, uint8_t ttl);
uint16_t checksum(const uint16_t *data, size_t size);

// reply
void update_ping_info(double time_rtt, ping_context_t *ctx);
void handle_reply(ping_context_t *ctx);

// helpers
void cleanup(ping_context_t *ctx);
void get_hostname_string_from_ip(const char *ip_str, char *dest_buf, size_t buf_len);
double compute_time_difference(const struct timespec past_time);

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
struct timespec get_probe_time(probe_record_t probes[MAX_TTL][MAX_PROBES], uint16_t seq);
uint8_t handle_responded_probes(ping_context_t *ctx, probe_index_t *oldest_i);
probe_index_t get_probe_index_from_sequence(uint16_t seq);

#endif