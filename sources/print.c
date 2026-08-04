#include "../includes/traceroute.h"

void print_start_string(struct ping_context ctx) {
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &((struct sockaddr_in *)ctx.destination_addrinfo->ai_addr)->sin_addr, ip_str, sizeof(ip_str));
    printf("traceroute to %s (%s), 30 hops max, 60 byte packets\n", ctx.arg_address, ip_str);
}