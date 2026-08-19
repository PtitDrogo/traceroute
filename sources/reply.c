#include "../includes/traceroute.h"

static void parse_icmp_reply(char *response, struct icmphdr **outer_icmp, icmp_packet_t **reply);
static void parse_udp_reply(char *response, struct icmphdr **outer_icmp, struct udphdr **reply_udp);

void handle_reply(tr_context_t *ctx) {
    struct sockaddr_in response_in;
    socklen_t src_len = sizeof(response_in);
    char response[UINT16_MAX];
    bool use_icmp = ctx->options.use_icmp;

    ssize_t bytes_received =
        recvfrom(ctx->recv_sock, &response, sizeof(response), 0, (struct sockaddr *)&response_in, &src_len);
    if (bytes_received == -1) {
        cleanup(ctx);
        exit(1);
    }
    char *ip_addr = inet_ntoa(response_in.sin_addr);

    struct icmphdr *outer_icmp;
    icmp_packet_t *reply;

    struct udphdr *reply_udp;
    if (use_icmp) {
        parse_icmp_reply(response, &outer_icmp, &reply);
    } else {
        parse_udp_reply(response, &outer_icmp, &reply_udp);
    }

    uint16_t seq = ntohs(use_icmp ? reply->header.un.echo.sequence : reply_udp->dest);
    probe_index_t probe_idx = get_decoded_probe_index_from_seq(seq);
    if (!ctx->options.use_icmp) {
        probe_idx.ttl = (ntohs(reply_udp->dest) - BASE_UDP_PORT) / ctx->options.max_probes_per_ttl;
        probe_idx.probe = (ntohs(reply_udp->dest) - BASE_UDP_PORT) % ctx->options.max_probes_per_ttl;
    }
    probe_record_t *probe = &ctx->probes[probe_idx.ttl][probe_idx.probe];
    if (probe->status == TIMEOUT)
        return;

    double time_slept_ms = ctx->time_slept_ms - probe->time_slept_when_sent_ms;
    double time_lost_by_dns_ms = ctx->time_lost_by_dns - probe->time_lost_by_dns_when_sent_ms;

    if (outer_icmp->type == ICMP_ECHOREPLY && ntohs(reply->header.un.echo.id) == (uint16_t)getpid() && use_icmp) {
        probe->time_rtt =
            compute_time_difference(*(struct timespec *)reply->payload) - time_slept_ms - time_lost_by_dns_ms;
        if (probe_idx.ttl < ctx->final_ttl_index) {
            ctx->final_ttl_index = probe_idx.ttl;
        }
    } else if (!use_icmp && outer_icmp->type == ICMP_DEST_UNREACH && outer_icmp->code == ICMP_PORT_UNREACH) {
        struct timespec sent_time = ctx->probes[probe_idx.ttl][probe_idx.probe].sent_at;
        probe->time_rtt = compute_time_difference(sent_time) - time_slept_ms - (time_lost_by_dns_ms / 2);
        if (probe_idx.ttl < ctx->final_ttl_index) {
            ctx->final_ttl_index = probe_idx.ttl;
        }

    } else if (outer_icmp->type == ICMP_TIME_EXCEEDED) {
        struct timespec sent_time = ctx->probes[probe_idx.ttl][probe_idx.probe].sent_at;
        probe->time_rtt = compute_time_difference(sent_time) - time_slept_ms - (time_lost_by_dns_ms / 2);
    } else {
        return;
    }
    ft_strlcpy(probe->ip, ip_addr, sizeof(probe->ip));

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

static void parse_udp_reply(char *response, struct icmphdr **outer_icmp, struct udphdr **reply_udp) {
    struct iphdr *ip = (struct iphdr *)response;
    int ip_hdr_len = ip->ihl * 4;
    *outer_icmp = (struct icmphdr *)(response + ip_hdr_len);

    struct iphdr *inner_ip = (struct iphdr *)(response + ip_hdr_len + sizeof(struct icmphdr));
    int inner_ip_hdr_len = inner_ip->ihl * 4;
    *reply_udp = (struct udphdr *)((char *)inner_ip + inner_ip_hdr_len);
}