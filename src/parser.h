#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

#include "lexer.h"

#define EXPECT(expr) do { if (!expr) return false; } while (false)
#define ERR_QUIT(msg) do { jas_err(msg); return false; } while (false)
#define ERR_FLUSH(msg) \
    do { jas_err(msg); token = flush_tok(); return false; } while (false)

/* passed to strtol to read in any base */
#define ANY_BASE 0

/** function prototypes **/
void parse(void);
void analyze(void);

#endif
