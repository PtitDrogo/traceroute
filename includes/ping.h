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
#define DEFAULT_MAX_TTL 30

#define HELP_STRING                                                                                                    \
    "Usage\n"                                                                                                          \
    "  ft_route [options] <destination>\n"                                                                             \
    "Options:\n"                                                                                                       \
    "  <destination>      DNS name or IP address\n"                                                                    \
    "  -V                 print version and exit\n"

// Contains the icmp header and the payload
struct ping_packet {
    struct icmphdr header;
    char payload[UINT16_MAX];
};

struct ping_result {
    struct timespec start_time;

    // These too can really easily be moved away from the struct tbh.
    char *arg_address;
    char resolved_address[NI_MAXHOST];
};

struct options {
    uint8_t max_ttl;
    // nothing yet !
};

struct ping_context {
    struct addrinfo *destination_addrinfo;
    struct ping_result res;
    struct options options;
    int sock;
    double interval_s;
    uint16_t seq;
};

// send
void send_packet(struct ping_packet *packet, struct ping_context *ctx);
void build_packet(struct ping_packet *packet, struct ping_context *ctx);
void update_packet(struct ping_packet *packet, struct ping_context *ctx);
void create_socket(struct options options, struct ping_context *ctx);
void update_socket(struct ping_context *ctx, uint8_t ttl);
uint16_t checksum(const uint16_t *data, size_t size);

// reply
void update_ping_info(double time_rtt, struct ping_context *ctx);
void handle_reply(struct ping_context *ctx);

// helpers
void cleanup(struct ping_context *ctx);
int32_t get_hostname_string_from_ip(const char *ip_str, char *dest_buf, size_t buf_len);
double compute_time_difference(const struct timespec past_time);

// parsing
void parse_flags(struct ping_context *ctx, int argc, char *argv[]);
long parse_num(struct ping_context *ctx, uint32_t max_range, char opt_char);

// Getting rid the \\ printing
void disable_echoctl(void);
void restore_termios(void);

#endif