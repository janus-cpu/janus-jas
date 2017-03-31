#ifndef UTIL_H
#define UTIL_H

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <strings.h>

// Portability for case-insensitive string comparison.
// HACK: strcasecmp/stricmp doesn't work in certain locales (e.g. Turkish)
#if defined(_WIN32) || defined(_WIN64)
    #define strcasecmp stricmp
#endif

#define strcaseeq(a,b) (strcasecmp((a),(b)) == 0)

/*
 * Copy while normalizing string to all caps.
 * Return number of bytes copied, including the nul character.
 */
int strcpyup(char * dest, const char * src);

#endif
