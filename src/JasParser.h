#ifndef PARSER_H
#define PARSER_H

#include <stdio.h>

#include "lexer.h"

/* probably the maximum line length */
#define MAX_LINE_LENGTH 500

/* error string: gets printed out for every error */
#define ERROR_STRING "\033[1m%s:%d: \033[31merror\033[39m: %s\n" \
                     ">>>>\033[0m%s\n"

#define ERR_QUIT(msg) \
    do { jas_err(msg, curr_line, lo_col, curr_col); return; } while (0)

/* passed to strtol to read in any base */
#define ANY_BASE 0

/** flex externs **/
extern FILE * yyin;
extern char * yytext;
extern int yylex(void);
extern int yyerr;
extern void yyerror(const char *);
extern char linebuf[];

/** function prototypes **/
void parse(FILE * in, FILE * out);
int isRegister(TokenType);

#endif
