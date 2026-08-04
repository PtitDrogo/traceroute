#include "../includes/ping.h"

void handle_reply(struct ping_context *ctx) {

    struct sockaddr_in response_in;
    socklen_t src_len = sizeof(response_in);
    char response[UINT16_MAX];

    ssize_t bytes_received =
        recvfrom(ctx->sock, &response, sizeof(response), 0, (struct sockaddr *)&response_in, &src_len);
    if (bytes_received == -1) {
        if (errno != EINTR) {
            cleanup(ctx);
            exit(1);
        }
        return;
    }
    
    // Gets the start of ICMP part of the packet, which is dynamic.
    struct iphdr *ip = (struct iphdr *)response;
    int ip_hdr_len = ip->ihl * 4;
    struct ping_packet *reply = (struct ping_packet *)(response + ip_hdr_len);
    
    char *ip_addr = inet_ntoa(response_in.sin_addr);
    
    double time_rtt = compute_time_difference(*(struct timespec *)reply->payload);
    if (!strlen(ctx->res.resolved_address)) {
        get_hostname_string_from_ip(ip_addr, ctx->res.resolved_address, sizeof(ctx->res.resolved_address),
                                    ctx->options.no_dns);
    }

    if (reply->header.type == ICMP_ECHOREPLY && ntohs(reply->header.un.echo.id) == (uint16_t)getpid()) {
        if (ctx->interval_s == INTERVAL_FLOOD_S) {
            write(STDOUT_FILENO, "\b", 1);
            update_ping_info(time_rtt, ctx);
            // Update the alarm part, maybe ?

        } else if (ctx->options.payload_size < sizeof(struct timespec)) {
            printf("%ld bytes from %s: icmp_seq=%d\n", bytes_received - ip_hdr_len, ctx->res.resolved_address,
                   ntohs(reply->header.un.echo.sequence));
            ctx->res.packets_received += 1;
            // print_pattern_reply(ctx, reply->payload);
        } else {
            update_ping_info(time_rtt, ctx);
            printf("%ld bytes from %s: icmp_seq=%d ttl=%d time=%.1f ms\n", bytes_received - ip_hdr_len,
                   ctx->res.resolved_address, ntohs(reply->header.un.echo.sequence), ip->ttl, time_rtt);
            // print_pattern_reply(ctx, reply->payload);
        }

    } else if (reply->header.type == ICMP_TIME_EXCEEDED) {
        printf("From %s icmp_seq=%d Time to live exceeded\n", ctx->res.resolved_address,
               ntohs(reply->header.un.echo.sequence));
    } else if (reply->header.type == ICMP_DEST_UNREACH) {
        printf("Destination Host Unreachable\n");
    }

    else {
        printf("Received ICMP packet: type=%d code=%d from %s (not an "
               "echo "
               "reply)\n",
               reply->header.type, reply->header.code, inet_ntoa(response_in.sin_addr));
    }
}

void handle_state(struct state *state, struct ping_context ctx) {
    if (state->printing) {
        print_curr_stats(ctx.res, ctx.options.payload_size);
        state->printing = false;
    }
    if (ctx.options.timeout_total_ms && compute_time_difference(ctx.res.start_time) >= ctx.options.timeout_total_ms) {
        state->running = false;
    }
    if (ctx.options.max_send && ctx.res.packets_transmitted >= ctx.options.max_send) {
        state->running = false;
    }
}