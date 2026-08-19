#include "../includes/traceroute.h"

void print_start_string(const ping_context_t *ctx) {
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &((struct sockaddr_in *)ctx->destination_addrinfo->ai_addr)->sin_addr, ip_str, sizeof(ip_str));
    printf("traceroute to %s (%s), 30 hops max, 60 byte packets\n", ctx->arg_address, ip_str);
}

void print_ttl_group(ping_context_t *ctx) {
    char curr_ip[INET_ADDRSTRLEN] = "";
    char resolved_address[NI_MAXHOST];

    printf("%d ", ctx->curr_ttl_to_print + 1);
    for (uint8_t i = 0; i < ctx->options.max_probes_per_ttl; i++) {
        probe_record_t *cur = &ctx->probes[ctx->curr_ttl_to_print][i];

        // printf("\nDebugging\n");
        // char resolved_address_debug[NI_MAXHOST];
        // get_hostname_string_from_ip(cur->ip, resolved_address_debug, sizeof(resolved_address_debug));
        // printf("%s %0.3f ms ", resolved_address, cur->time_rtt);
        // printf("Debugging end\n");

        // printf("%s, ", cur->ip);
        if (cur->status == TIMEOUT) { // strcmp(resolved_address, "TIMEOUT")
            printf("* ");
        } else if (strcmp(cur->ip, curr_ip) == 0) {
            printf("%0.3f ms ", cur->time_rtt);
        } else {
            if (ctx->options.skip_dns) {
                printf("%s %0.3f ms ", cur->ip, cur->time_rtt);
            } else {
                struct timespec start, end;
                clock_gettime(CLOCK_MONOTONIC, &start);
                get_hostname_string_from_ip(cur->ip, resolved_address, sizeof(resolved_address));
                clock_gettime(CLOCK_MONOTONIC, &end);

                double dns_lookup_time_ms =
                    (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
                ctx->time_lost_by_dns += dns_lookup_time_ms;
                printf("%s %0.3f ms ", resolved_address, cur->time_rtt);
            }
            ft_strlcpy(curr_ip, cur->ip, sizeof(curr_ip));
        }
    }
    printf("\n");
}

bool is_ttl_group_ready(const ping_context_t *ctx) {
    for (uint8_t i = 0; i < ctx->options.max_probes_per_ttl; i++) {
        probe_status s = ctx->probes[ctx->curr_ttl_to_print][i].status;
        if (s == NOT_SENT || s == PENDING)
            return false;
    }
    return true;
}

void print_ready_ttl_groups(ping_context_t *ctx) {
    while (is_ttl_group_ready(ctx)) {
        print_ttl_group(ctx);
        ctx->curr_ttl_to_print += 1;
        if (ctx->curr_ttl_to_print > ctx->final_ttl_index) { // Do not touch this ! :)
            cleanup(ctx);
            exit(EXIT_SUCCESS);
        }
    }
    return;
}
