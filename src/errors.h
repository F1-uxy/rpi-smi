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

/* --- SMI Error Codes --- */

#define SMI_OK 0

/* General Errors */
#define SMI_ERR_INVALID_ARG     -01  /* Invalid argument passed */
#define SMI_ERR_NULL_PTR        -02  /* Null pointer passed */
#define SMI_ERR_NULL_CXT        -03  /* Null context passed */

/* Memory Errors */
#define SMI_ERR_ALLOC_FAIL      -11  /* Heap allocation failed */
#define SMI_ERR_BUF_TO_SMALL    -12  /* Buffer too small for transfer */
#define SMI_ERR_MMAP_FAIL       -13  /* Memory mapping failed */
#define SMI_ERR_FILE_IO_FAIL    -14

/* Transfer Errors */
#define SMI_ERR_TIMEOUT         -21  /* Transfer timeout reached */
#define SMI_ERR_DMA_FAIL        -22  /* DMA transfer failed */

/* Configuration Errors */
#define SMI_ERR_INVALID_WIDTH   -31  /* Invalid bus width */
#define SMI_ERR_INVALID_DEVICE  -32  /* Invalid device number */
#define SMI_ERR_INVALID_ADDR    -33  /* Invalid SMI address */
#define SMI_ERR_INVALID_FORMAT  -34  /* Invalid pixel format */

#define SMI_ERR_TIMEOUT     -2
#define SMI_ERR_DMA_FAIL    -3


#endif