#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

#include "Registers.h"

#define REG_SIZE 4
#define REG_BASE 10

static int isShortRegister(char endchar) {
    return (endchar == 'a' ||
            endchar == 'b' ||
            endchar == 'c' ||
            endchar == 'd');
}

RegisterId getRegisterId(const char * name) {
    RegisterId id = 0;
    char * endptr;

    /* move 1 byte past the 'r' */
    name = name + 1;

    /* check for aliased registers */
    switch (*name) {
        case 's': return 15;
        case 'e': id = 16; name++; break;
        case 'k': id = 24; name++; break;
    }


    errno = 0;
    id += (RegisterId) strtol(name, &endptr, REG_BASE);

    if (isShortRegister(*endptr)) {
        id *= REG_SIZE;
        id += (*endptr - 'a') + 1;
    }

    return id;
}
