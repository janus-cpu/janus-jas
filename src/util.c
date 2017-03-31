#include "util.h"

/*
 * Copy while normalizing string to all caps.
 * Return number of bytes copied, including the nul character.
 */
int strcpyup(char * dest, const char * src) {
    int i = 0;
    while ((dest[i] = toupper(src[i])))
        i++;
    return i + 1;
}
