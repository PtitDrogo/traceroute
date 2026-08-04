#include "../includes/ping.h"

void print_stats(struct ping_result res, uint16_t payload_size) {
    double time_elasped = compute_time_difference(res.start_time);
    double mean = res.rrt_in.sum_rtt / res.packets_received;
    double variance = (res.rrt_in.sum_rtt_squared / res.packets_received) - pow(mean, 2);
    double mdev = sqrt(variance);

    int lost = res.packets_transmitted - res.packets_received;
    int loss_percent = res.packets_transmitted > 0 ? (lost * 100) / res.packets_transmitted : 0;

    printf("\n--- %s ping statistics ---\n", res.arg_address);
    printf("%d packets transmitted, %d received, %d%% packet loss, time: %.0f ms\n", res.packets_transmitted,
           res.packets_received, loss_percent, time_elasped);

    if (res.packets_received >= 1 && payload_size > sizeof(struct timespec)) {
        printf("rtt min/avg/max/mdev = %.3f/%.3f/%.3f/%.3f ms\n", res.rrt_in.min_time, mean, res.rrt_in.max_time, mdev);
    }
}

void print_curr_stats(struct ping_result res, uint16_t payload_size) {
    int lost = res.packets_transmitted - res.packets_received;
    int loss_percent = res.packets_transmitted > 0 ? (lost * 100) / res.packets_transmitted : 0;

    if (res.packets_received == 0 || payload_size < 16) {
        printf("%d/%d packets, %d%% loss\n", res.packets_received, res.packets_transmitted, loss_percent);
        return;
    }

    printf("%d/%d packets, %d%% loss, min/avg/ewma/max = %.3f/%.3f/%.3f/%.3f ms\n", res.packets_received,
           res.packets_transmitted, loss_percent, res.rrt_in.min_time, res.rrt_in.sum_rtt / res.packets_received,
           res.rrt_in.ewma, res.rrt_in.max_time);
}

void print_socket_verbose(const struct ping_context *ctx) {
    printf("ping: sock.fd: %d (socktype: SOCK_RAW), hints.ai_family: AF_INET\n", ctx->sock);
}

void print_addr_verbose(const struct ping_context *ctx) {
    const struct addrinfo *ai = ctx->destination_addrinfo;

    if (ai == NULL)
        return;

    const char *fam_str = (ai->ai_family == AF_INET)    ? "AF_INET"
                          : (ai->ai_family == AF_INET6) ? "AF_INET6"
                                                        : "AF_UNKNOWN";

    printf("ai->ai_family: %s, ai->ai_canonname: '%s'\n", fam_str,
           ai->ai_canonname ? ai->ai_canonname : ctx->res.arg_address);
}