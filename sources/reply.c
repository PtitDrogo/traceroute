#include "../includes/traceroute.h"

static void parse_icmp_reply(char *response, struct icmphdr **outer_icmp, icmp_packet_t **reply);
static void parse_udp_reply(char *response, struct icmphdr **outer_icmp, struct udphdr **reply_udp);

void handle_reply(ping_context_t *ctx) {
    printf("Hello\n ?");
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

    // We dont actually
    struct udphdr *reply_udp;
    if (ctx->options.use_icmp) {
        parse_icmp_reply(response, &outer_icmp, &reply);
    } else {
        parse_udp_reply(response, &outer_icmp, &reply_udp);
    }
    // Im not actually gonna be able to read from the payload
    // All I want is the sequence number of the response

    uint16_t seq;

    if (ctx->options.use_icmp) {
        seq = ntohs(reply->header.un.echo.sequence);
    } else {
        seq = ntohs(reply_udp->dest);
    }

    probe_index_t probe_idx;

    if (ctx->options.use_icmp) {
        probe_idx = get_probe_index_from_sequence(seq);
    } else {
        probe_idx = get_probe_index_from_port(seq);
    }

    probe_record_t *probe = &ctx->probes[probe_idx.ttl][probe_idx.probe];

    printf("recv: type=%d code=%d port=%d -> ttl=%d probe=%d status=%d\n", outer_icmp->type, outer_icmp->code, seq,
           probe_idx.ttl, probe_idx.probe, ctx->probes[probe_idx.ttl][probe_idx.probe].status);

    if (probe->status == TIMEOUT)
        return; // This is a ping that we got too late, we ignore it !

    // All of this is kinda confusing, but bottom line is:
    //  -> 99% of the time we receive a time exceeded, we want sed number to compute which probe in our probe struct it
    //  is
    //  -> sometime it will be a success, if were ICMP, we read from the payload to get the time because were tryhards.

    if (outer_icmp->type == ICMP_ECHOREPLY && ntohs(reply->header.un.echo.id) == (uint16_t)getpid() &&
        ctx->options.use_icmp) {
        probe->time_rtt = compute_time_difference(*(struct timespec *)reply->payload);
        // probe->time_rtt = compute_time_difference(*(struct timespec *)reply->payload);
        if (probe_idx.ttl < ctx->final_ttl_index) {
            ctx->final_ttl_index = probe_idx.ttl;
            printf("Setting end reply to %d!\n", ctx->final_ttl_index);
        }
    } else if (!ctx->options.use_icmp && outer_icmp->type == ICMP_DEST_UNREACH &&
               outer_icmp->code == ICMP_PORT_UNREACH) {
        struct timespec sent_time = ctx->probes[probe_idx.ttl][probe_idx.probe].sent_at;
        probe->time_rtt = compute_time_difference(sent_time);
        if (probe_idx.ttl < ctx->final_ttl_index) {
            ctx->final_ttl_index = probe_idx.ttl;
            printf("Setting end reply to %d!\n", ctx->final_ttl_index);
        }

    } else if (outer_icmp->type == ICMP_TIME_EXCEEDED) {
        if (ctx->options.use_icmp) {
            probe->time_rtt = compute_time_difference(get_probe_time(ctx->probes, seq));
        } else {
            struct timespec sent_time = ctx->probes[probe_idx.ttl][probe_idx.probe].sent_at;
            probe->time_rtt = compute_time_difference(sent_time);
        }
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

// Gets to the end of ip header, cast this to whatever type you think you have.
// static void *get_end_of_ip_header(char *response) {
//     struct iphdr *ip = (struct iphdr *)response;
//     int ip_hdr_len = ip->ihl * 4;
//     return response + ip_hdr_len;
// }