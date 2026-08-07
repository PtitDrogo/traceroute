#include "../includes/traceroute.h"

static void parse_icmp_reply(char *response, struct icmphdr **outer_icmp, icmp_packet_t **reply);

void handle_reply(ping_context_t *ctx) {

    struct sockaddr_in response_in;
    socklen_t src_len = sizeof(response_in);
    char response[UINT16_MAX];

    ssize_t bytes_received =
        recvfrom(ctx->recv_sock, &response, sizeof(response), 0, (struct sockaddr *)&response_in, &src_len);
    if (bytes_received == -1) {
        cleanup(ctx);
        exit(1);
    }
    char *ip_addr = inet_ntoa(response_in.sin_addr);
    get_hostname_string_from_ip(ip_addr, ctx->res.resolved_address, sizeof(ctx->res.resolved_address));

    struct icmphdr *outer_icmp;
    icmp_packet_t *reply;
    parse_icmp_reply(response, &outer_icmp, &reply);

    uint16_t seq = ntohs(reply->header.un.echo.sequence);
    probe_index_t probe_idx = get_probe_index_from_sequence(seq);
    probe_record_t *probe = &ctx->probes[probe_idx.ttl][probe_idx.probe];
    if (probe->status == TIMEOUT)
        return; // This is a ping that we got too late, we ignore it !

    if (outer_icmp->type == ICMP_ECHOREPLY && ntohs(reply->header.un.echo.id) == (uint16_t)getpid()) {
        probe->time_rtt = compute_time_difference(*(struct timespec *)reply->payload);
        if (probe_idx.ttl < ctx->final_ttl) {
            ctx->final_ttl = probe_idx.ttl;
            printf("Setting end reply to %d!\n", ctx->final_ttl);
        }
    } else if (outer_icmp->type == ICMP_TIME_EXCEEDED) {
        probe->time_rtt = compute_time_difference(get_probe_time(ctx->probes, seq));
    }
    ft_strlcpy(probe->resolved_address, ctx->res.resolved_address, sizeof(probe->resolved_address));

    // This can probe and and will often be the oldest probe, but that should be handled in the timeout function.
    probe->status = RESPONDED;
    return;
}

//[IP of replier][ICMP header of replier]<PAYLOAD:[IP Header of sender][ICMP Header of sender]>
//               <- We are here                   <- then here          <- then here, the goal !
static void parse_icmp_reply(char *response, struct icmphdr **outer_icmp, icmp_packet_t **reply) {
    struct iphdr *ip = (struct iphdr *)response;
    int ip_hdr_len = ip->ihl * 4;
    *outer_icmp = (struct icmphdr *)(response + ip_hdr_len);

    if ((*outer_icmp)->type == ICMP_TIME_EXCEEDED || (*outer_icmp)->type == ICMP_DEST_UNREACH) {
        struct iphdr *inner_ip = (struct iphdr *)(response + ip_hdr_len + sizeof(struct icmphdr));
        int inner_ip_hdr_len = inner_ip->ihl * 4;
        *reply = (icmp_packet_t *)((char *)inner_ip + inner_ip_hdr_len);
    } else {
        *reply = (icmp_packet_t *)(response + ip_hdr_len);
    }
}