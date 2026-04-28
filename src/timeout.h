#ifndef TIMEOUT_H
#define TIMEOUT_H

#include <time.h>
#include <sched.h>

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

/*
 * Returns: 0 = hard spin
 *          1 = CPU yield hint
 *          2 = sched_yield
 *          3 = nanosleep             
 */
static inline int timeout_spin_tier(int spin, int hard, int yield, int soft)
{
    if (spin < hard)        return 0;
    if (spin < yield)       return 1;
    if (spin < soft)        return 2;
    return 3;
}

static inline bool timeout_apply(smi_timeout_ns deadline, int tier)
{
    switch (tier)
    {
    case 0:
        break;
    case 1:
        __asm__ volatile("nop"   ::: "memory");
        break;
    case 2:
        if(timeout_complete(deadline)) return true;
        sched_yield();
        break;
    case 3:
        if(timeout_complete(deadline)) return true;
        struct timespec ts = { .tv_sec = 0, .tv_nsec = 100000 }; 
        nanosleep(&ts, NULL);
        if(timeout_complete(deadline)) return true;
        break;
    default:
        break;
    }

    return false;
}

#endif