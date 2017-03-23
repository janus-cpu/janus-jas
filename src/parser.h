#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

#include "lexer.h"

#define ERR_QUIT(msg) \
    do { jas_err(msg, curr_line, lo_col, curr_col); return; } while (0)

/* passed to strtol to read in any base */
#define ANY_BASE 0

/** function prototypes **/
void assemble(FILE * in, FILE * out);
int isRegister(TokenType);

#endif
