#include "../includes/ping.h"

uint16_t checksum(const uint16_t *data, size_t size) {
    uint32_t sum = 0;
    size_t data_len = size / 2;
    size_t i;

    for (i = 0; i < data_len; i++)
        sum += data[i];

    if (size & 1) {
        const uint8_t *tail = (const uint8_t *)data;
        sum += tail[size - 1];
    }

    while (sum >> 16)
        sum = (sum & 0xFFFF) + (sum >> 16);

    return (uint16_t)~sum;
}