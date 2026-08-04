#include "../includes/traceroute.h"

static void parse_icmp_reply(char *response, struct icmphdr **outer_icmp, struct ping_packet **reply);

void handle_reply(struct ping_context *ctx) {

    struct sockaddr_in response_in;
    socklen_t src_len = sizeof(response_in);
    char response[UINT16_MAX];

    ssize_t bytes_received =
        recvfrom(ctx->sock, &response, sizeof(response), 0, (struct sockaddr *)&response_in, &src_len);
    if (bytes_received == -1) {
        bool is_timeout = errno == EAGAIN || errno == EWOULDBLOCK;
        if (is_timeout) {
            printf("*\n");
            return;
        }
        cleanup(ctx);
        exit(1);
    }
    char *ip_addr = inet_ntoa(response_in.sin_addr);
    get_hostname_string_from_ip(ip_addr, ctx->res.resolved_address, sizeof(ctx->res.resolved_address));

    struct icmphdr *outer_icmp;
    struct ping_packet *reply;
    parse_icmp_reply(response, &outer_icmp, &reply);

    if (outer_icmp->type == ICMP_ECHOREPLY && ntohs(reply->header.un.echo.id) == (uint16_t)getpid()) {
        double time_rtt = compute_time_difference(*(struct timespec *)reply->payload);
        printf("%s %f ms\n", ctx->res.resolved_address, time_rtt);
        cleanup(ctx);
        exit(EXIT_SUCCESS);
    } else if (outer_icmp->type == ICMP_TIME_EXCEEDED) {
        double time_rtt = compute_time_difference(get_probe_time(ctx->probes, ntohs(reply->header.un.echo.sequence)));
        printf("%s %f ms\n", ctx->res.resolved_address, time_rtt);

    } else {
        printf("Error, Didnt get expected packet\n");
    }
    return;
}


//[IP of replier][ICMP header of replier]<PAYLOAD:[IP Header of sender][ICMP Header of sender]>
//               <- We are here                   <- then here          <- then here, the goal !
static void parse_icmp_reply(char *response, struct icmphdr **outer_icmp, struct ping_packet **reply) {
    struct iphdr *ip = (struct iphdr *)response;
    int ip_hdr_len = ip->ihl * 4;
    *outer_icmp = (struct icmphdr *)(response + ip_hdr_len);

    if ((*outer_icmp)->type == ICMP_TIME_EXCEEDED || (*outer_icmp)->type == ICMP_DEST_UNREACH) {
        struct iphdr *inner_ip = (struct iphdr *)(response + ip_hdr_len + sizeof(struct icmphdr));
        int inner_ip_hdr_len = inner_ip->ihl * 4;
        *reply = (struct ping_packet *)((char *)inner_ip + inner_ip_hdr_len);
    } else {
        *reply = (struct ping_packet *)(response + ip_hdr_len);
    }
}