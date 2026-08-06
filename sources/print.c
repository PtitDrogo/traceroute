#include "../includes/traceroute.h"

void print_start_string(const ping_context_t *ctx) {
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &((struct sockaddr_in *)ctx->destination_addrinfo->ai_addr)->sin_addr, ip_str, sizeof(ip_str));
    printf("traceroute to %s (%s), 30 hops max, 60 byte packets\n", ctx->arg_address, ip_str);
}

void print_ttl_group(ping_context_t *ctx) {
    char *curr_addrr = "";

    printf("%d ", ctx->curr_ttl_to_print + 1);
    for (uint8_t i = 0; i < ctx->options.max_probes; i++) {
        probe_record_t cur = ctx->probes[ctx->curr_ttl_to_print][i];
        if (strcmp(cur.resolved_address, "* ") == 0) {
            printf("* ");
        } else if (strcmp(cur.resolved_address, curr_addrr) == 0) {
            printf("%0.3f ms ", cur.time_rtt);
        } else {
            printf("%s %0.3f ms ", cur.resolved_address, cur.time_rtt);
            curr_addrr = cur.resolved_address;
        }
    }
    printf("\n");
    return;
}

bool is_ttl_group_ready(const ping_context_t *ctx) {
    for (uint8_t i = 0; i < ctx->options.max_probes; i++) {
        if (ctx->probes[ctx->curr_ttl_to_print][i].status == PENDING)
            return false;
    }
    return true;
}

void print_ready_ttl_groups(ping_context_t *ctx) {
    while (is_ttl_group_ready(ctx)) {
        print_ttl_group(ctx);
        ctx->curr_ttl_to_print += 1;
        if (ctx->curr_ttl_to_print > ctx->final_ttl) { //Do not touch this ! :)
            cleanup(ctx);
            exit(EXIT_SUCCESS);
        }
    }
    return;
}
