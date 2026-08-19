#include "../includes/traceroute.h"

double sleep_and_measure(double time_to_sleep_ms) {
    printf("time to sleep ms %f\n", time_to_sleep_ms);
    long total_nsec = (long)(time_to_sleep_ms * 1000.0 * 1000.0);
    struct timespec pause = {
        .tv_sec = total_nsec / 1000000000L,
        .tv_nsec = total_nsec % 1000000000L
    };
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    nanosleep(&pause, NULL);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double slept_ms = (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
    printf("I just slept %f ms\n", slept_ms);
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