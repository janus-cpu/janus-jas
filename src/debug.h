#ifndef DEBUG_H
#define DEBUG_H

#include <stdbool.h>

/* for debug messages */
extern bool debug_on;

#define DEBUG(m,...) if (debug_on) \
    fprintf(stderr, "[DEBUG] "m"\n", ##__VA_ARGS__)

#endif
