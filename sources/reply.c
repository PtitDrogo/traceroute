#include "../includes/traceroute.h"

static void parse_icmp_reply(char *response, struct icmphdr **outer_icmp, struct ping_packet **reply);

void handle_reply(struct ping_context *ctx) {

    struct sockaddr_in response_in;
    socklen_t src_len = sizeof(response_in);
    char response[UINT16_MAX];

    ssize_t bytes_received =
        recvfrom(ctx->sock, &response, sizeof(response), 0, (struct sockaddr *)&response_in, &src_len);
    if (bytes_received == -1) {
        cleanup(ctx);
        exit(1);
    }
    char *ip_addr = inet_ntoa(response_in.sin_addr);
    get_hostname_string_from_ip(ip_addr, ctx->res.resolved_address, sizeof(ctx->res.resolved_address));

    struct icmphdr *outer_icmp;
    struct ping_packet *reply;
    parse_icmp_reply(response, &outer_icmp, &reply);

    uint16_t seq = ntohs(reply->header.un.echo.sequence);
    struct probe_index probe_idx = get_probe_index_from_sequence(seq);
    struct probe_record *probe = &ctx->probes[probe_idx.ttl][probe_idx.probe];
    if (probe->status == TIMEOUT)
        return; // This is a ping that we got too late, we ignore it !

    if (outer_icmp->type == ICMP_ECHOREPLY && ntohs(reply->header.un.echo.id) == (uint16_t)getpid()) {
        double time_rtt = compute_time_difference(*(struct timespec *)reply->payload);

        // Check if one of the probes in our probe group already printed the thing.
        bool did_print = false;
        bool all_done = true;
        // we figure out if we already printed, and if everything is done
        for (int i = 0; i < MAX_PROBES; i++) {
            if (ctx->probes[probe_idx.ttl][i].status == RESPONDED) {
                did_print = true;
            }
            if (probe_idx.probe != i && ctx->probes[probe_idx.ttl][i].status == PENDING) {
                all_done = false;
            }
        }
        if (did_print) {
            printf("%f ms ", time_rtt);
        } else {
            printf("%s %f ms ", ctx->res.resolved_address, time_rtt);
        }

        if (all_done) {
            printf("\n");
            cleanup(ctx);
            exit(EXIT_SUCCESS);
        }

    } else if (outer_icmp->type == ICMP_TIME_EXCEEDED) {
        double time_rtt = compute_time_difference(get_probe_time(ctx->probes, seq));
        // Check if one of the probes in our probe group already printed the thing.
        bool did_print = false;
        bool all_done = true;
        // we figure out if we already printed, and if everything is done
        for (int i = 0; i < MAX_PROBES; i++) {
            if (ctx->probes[probe_idx.ttl][i].status == RESPONDED) {
                did_print = true;
            }
            if (probe_idx.probe != i && ctx->probes[probe_idx.ttl][i].status == PENDING) {
                all_done = false;
            }
        }
        if (did_print) {
            printf("%f ms ", time_rtt);
        } else {
            printf("%s %f ms ", ctx->res.resolved_address, time_rtt);
        }

        if (all_done) {
            printf("\n");
        }
    } else {
        printf("Error, Didnt get expected packet\n");
    }
    probe->status = RESPONDED;
    // BUT WHAT IF THAT GUY WAS THE OLDEST PROBE GOD DAMNIT I gotta update the new oldest probe.
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