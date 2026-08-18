#include "../includes/traceroute.h"

static void parse_icmp_reply(char *response, struct icmphdr **outer_icmp, icmp_packet_t **reply);
static void parse_udp_reply(char *response, struct icmphdr **outer_icmp, struct udphdr **reply_udp);
static uint8_t get_sender_ttl(char *response);

void handle_reply(ping_context_t *ctx) {
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
    // struct timespec before_dns;
    // clock_gettime(CLOCK_REALTIME, &before_dns);
    // get_hostname_string_from_ip(ip_addr, ctx->res.resolved_address, sizeof(ctx->res.resolved_address));
    // double dns_lookup_time = compute_time_difference(before_dns);

    // printf("Dns lookup for %s took: %f\n", ctx->res.resolved_address, dns_lookup_time);

    // ft_strlcpy(ctx->res.resolved_address, ip_addr, sizeof(ip_addr)); //Debug line to skip dns lookup.

    struct icmphdr *outer_icmp;
    icmp_packet_t *reply;

    struct udphdr *reply_udp;
    if (use_icmp) {
        parse_icmp_reply(response, &outer_icmp, &reply);
    } else {
        parse_udp_reply(response, &outer_icmp, &reply_udp);
    }
    // Im not actually gonna be able to read from the payload
    // All I want is the sequence number of the response

    uint16_t seq = ntohs(use_icmp ? reply->header.un.echo.sequence : reply_udp->dest);
    // uint16_t seqtest = reply->header.un.echo.sequence;
    probe_index_t probe_idx = get_decoded_probe_index_from_seq(seq);
    if (!ctx->options.use_icmp) {
        probe_idx.ttl = get_sender_ttl(response) - 1;
        probe_idx.probe = ntohs(reply_udp->dest) - BASE_UDP_PORT;
    }
    // printf("ttl %d, probe %d\n", probe_idx.ttl + 1, probe_idx.probe);
    // probe_index_t probe_idxl;
    // Technically speaking we could get really unlucky and receive a random ping here and it crashes our shit.
    probe_record_t *probe = &ctx->probes[probe_idx.ttl][probe_idx.probe];

    // printf("recv: type=%d code=%d port=%d -> ttl=%d probe=%d status=%d\n", outer_icmp->type, outer_icmp->code, seq,
    //        probe_idx.ttl, probe_idx.probe, ctx->probes[probe_idx.ttl][probe_idx.probe].status);

    if (probe->status == TIMEOUT)
        return; // This is a ping that we got too late, we ignore it !

    double time_slept_ms = ctx->time_slept_ms - probe->time_slept_when_sent_ms;
    double time_lost_by_dns_ms = ctx->time_lost_by_dns - probe->time_lost_by_dns_when_sent_ms;
    // printf("ctx->time_slept_ms - probe->time_slept_when_sent_ms = time slept ms: %f -%f = %f ", ctx->time_slept_ms,
    //        probe->time_slept_when_sent_ms, time_slept_ms);
    // All of this is kinda confusing, but bottom line is:
    //  -> 99% of the time we receive a time exceeded, we want sed number to compute which probe in our probe struct it
    //  is
    //  -> sometime it will be a success, if were ICMP, we read from the payload to get the time because were tryhards.

    if (outer_icmp->type == ICMP_ECHOREPLY && ntohs(reply->header.un.echo.id) == (uint16_t)getpid() && use_icmp) {
        probe->time_rtt = compute_time_difference(*(struct timespec *)reply->payload) - time_slept_ms - time_lost_by_dns_ms;
        if (probe_idx.ttl < ctx->final_ttl_index) {
            ctx->final_ttl_index = probe_idx.ttl;
            printf("Setting end reply to %d!\n", ctx->final_ttl_index);
        }
    } else if (!use_icmp && outer_icmp->type == ICMP_DEST_UNREACH && outer_icmp->code == ICMP_PORT_UNREACH) {
        struct timespec sent_time = ctx->probes[probe_idx.ttl][probe_idx.probe].sent_at;
        probe->time_rtt = compute_time_difference(sent_time) - time_slept_ms - (time_lost_by_dns_ms / 2);
        if (probe_idx.ttl < ctx->final_ttl_index) {
            ctx->final_ttl_index = probe_idx.ttl;
            printf("Setting end reply to %d!\n", ctx->final_ttl_index);
        }

    } else if (outer_icmp->type == ICMP_TIME_EXCEEDED) {
        struct timespec sent_time = ctx->probes[probe_idx.ttl][probe_idx.probe].sent_at;
        probe->time_rtt = compute_time_difference(sent_time) - time_slept_ms - (time_lost_by_dns_ms / 2);
    } else {
        return; // Unrelated packet, we do not touch.
    }
    // ft_strlcpy(probe->resolved_address, ctx->res.resolved_address, sizeof(probe->resolved_address));
    ft_strlcpy(probe->ip, ip_addr, sizeof(probe->ip));

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

static void parse_udp_reply(char *response, struct icmphdr **outer_icmp, struct udphdr **reply_udp) {
    struct iphdr *ip = (struct iphdr *)response;
    int ip_hdr_len = ip->ihl * 4;
    *outer_icmp = (struct icmphdr *)(response + ip_hdr_len);

    // UDP probes only ever get ICMP *error* replies back (never a "success" reply type),
    // so the inner original packet is always present under TIME_EXCEEDED / DEST_UNREACH.
    struct iphdr *inner_ip = (struct iphdr *)(response + ip_hdr_len + sizeof(struct icmphdr));
    int inner_ip_hdr_len = inner_ip->ihl * 4;
    *reply_udp = (struct udphdr *)((char *)inner_ip + inner_ip_hdr_len);
}

static uint8_t get_sender_ttl(char *response) {
    struct iphdr *ip = (struct iphdr *)response;
    int ip_hdr_len = ip->ihl * 4;
    struct iphdr *inner_ip = (struct iphdr *)(response + ip_hdr_len + sizeof(struct icmphdr));
    return inner_ip->ttl;
}

// Gets to the end of ip header, cast this to whatever type you think you have.
// static void *get_end_of_ip_header(char *response) {
//     struct iphdr *ip = (struct iphdr *)response;
//     int ip_hdr_len = ip->ihl * 4;
//     return response + ip_hdr_len;
// }