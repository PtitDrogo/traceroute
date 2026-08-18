#include "../includes/traceroute.h"

// Still the given amount of nanosecond and returns how long it actually slept in ms
double sleep_and_measure(long time_to_sleep_nsec) {
    struct timespec pause = {.tv_sec = 0, .tv_nsec = time_to_sleep_nsec};
    struct timespec start, end;

    clock_gettime(CLOCK_MONOTONIC, &start);

    nanosleep(&pause, NULL);

    clock_gettime(CLOCK_MONOTONIC, &end);

    double slept_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;

    return slept_ms;
}

// returns the time difference between the given timespec and now.
double compute_time_difference(struct timespec past_time) {
    struct timespec end_time;
    clock_gettime(CLOCK_REALTIME, &end_time);

    const struct timespec start_time = past_time;

    double rtt = (end_time.tv_sec - start_time.tv_sec) * 1000.0 + (end_time.tv_nsec - start_time.tv_nsec) / 1000000.0;
    return rtt;
}