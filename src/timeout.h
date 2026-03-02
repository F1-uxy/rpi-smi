#ifndef TIMEOUT_H
#define TIMEOUT_H

#include <time.h>

#if !defined(_POSIX_VERSION) || _POSIX_VERSION < 199309L
    #error "POSIX verison is insufficient for the required clock_gettime()"
#endif

typedef struct timespec smi_timeout;
typedef int64_t smi_timeout_ns;

/* Returns the timeout deadline */
static inline smi_timeout_ns start_timeout(double seconds)
{
    smi_timeout now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return (int64_t)now.tv_sec * 1000000000LL +
           now.tv_nsec +
           (int64_t)(seconds * 1000000000.0);
}

static inline int timeout_complete(smi_timeout_ns deadline)
{
    smi_timeout now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    int64_t now_ns = (int64_t)now.tv_sec * 1000000000LL + now.tv_nsec;

    return now_ns >= deadline;
}

#endif