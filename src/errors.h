#ifndef ERRORS_H
#define ERRORS_H

#include <unistd.h>


#ifdef SMI_COLOURFULL_ERRORS
    #define ERROR(e) \
        fprintf(stderr, "%sERROR: %s%s\n", isatty(fileno(stderr)) ? "\x1b[31m" : "", e, isatty(fileno(stderr)) ? "\x1b[0m" : "")

    #define WARNING(w) \
        fprintf(stderr, "%sWARNING: %s%s\n", isatty(fileno(stderr)) ? "\x1b[33m" : "", w, isatty(fileno(stderr)) ? "\x1b[0m" : "")
#else
    #define ERROR(e) (fprintf(stderr, "ERROR: %s\n", e))

    #define WARNING(w) (fprintf(stderr, "WARNING: %s\n", w))
#endif

#ifdef SMI_VERBOSE
    #define LOG(msg) (fprintf(stdout, "SMI: %s\n", msg))
#else
    #define LOG(msg)
#endif

#endif