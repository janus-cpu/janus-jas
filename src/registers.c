#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "registers.h"

#define REG_SIZE 4
#define REG_BASE 10

int is_short_register(char endchar) {
    return (endchar == 'a' ||
            endchar == 'b' ||
            endchar == 'c' ||
            endchar == 'd');
}

/*
 * Get the corresponding register id for the given name.
 */
RegisterId register_id(const char * name) {
    RegisterId id = 0;
    char * endptr;

    /* move 1 byte past the 'r' */
    name = name + 1;

    /* check for aliased registers */
    switch (*name) {
        case 'r': return 14;
        case 's': return 15;
    }

    errno = 0;
    id += (RegisterId) strtol(name, &endptr, REG_BASE);

    if (is_short_register(*endptr)) {
        id *= REG_SIZE;
        id += (*endptr - 'a');
    }

    return id;
}
