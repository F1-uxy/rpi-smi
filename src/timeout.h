#ifndef TIMEOUT_H
#define TIMEOUT_H

#include <time.h>

#if !defined(_POSIX_VERSION) || _POSIX_VERSION < 199309L
    #error "POSIX verison is insufficient for the required clock_gettime()"
#endif

static inline void start_timeout(struct timespec* start)
{
    clock_gettime(CLOCK_MONOTONIC, start);
}

static inline int timeout_complete(struct timespec* start, double timeout)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    double elapsed = (now.tv_sec - start->tv_sec) +
                     (now.tv_nsec - start->tv_nsec)/1e9;

    return elapsed >= timeout;
}


#endif