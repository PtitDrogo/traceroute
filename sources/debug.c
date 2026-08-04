
#include "../includes/ping.h"

// prints the payload response from the replier.
void print_pattern_reply(struct ping_context *ctx, void *payload) {
    if (ctx->options.pattern.len == 0) {
        return;
    }
    size_t start_idx = (ctx->options.payload_size >= sizeof(struct timespec)) ? sizeof(struct timespec) : 0;
    printf("Checking pattern from sender : ");
    for (size_t i = start_idx; i < ctx->options.payload_size; i++) {
        size_t pattern_idx = (i - start_idx) % ctx->options.pattern.len;
        unsigned char expected = ctx->options.pattern.buf[pattern_idx];
        unsigned char actual = ((unsigned char *)payload)[i];
        printf("%02x", actual);
        if (actual != expected) {
            printf("(!=%02x)", expected);
        }
    }
    printf("\n");
}