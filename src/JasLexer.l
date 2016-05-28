%{

#include <stdio.h>
#include "JasParser.h"

extern int fileno(FILE *); /* shut up the compiler */
extern char * infilename;

/* count line numbers, store lines */
int lineno = 1;
int yycolumn = 1;
char linebuf[MAX_LINE_LENGTH];

struct YYLTYPE {
    int first_line;
    int first_column;
    int last_line;
    int last_column;
} yylloc;

#define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno;\
    yylloc.first_column = yycolumn; yylloc.last_column = yycolumn + yyleng - 1;\
    yycolumn += yyleng;

/* return 1 to prevent assumption that we're scanning another file */
/* TODO: this will change soon when we handle multiple files!      */
int yywrap(void) {
    return 1;
}

/* error output */
void yyerror(const char * msg) {
    fprintf(stderr, ERROR_STRING, infilename, lineno, msg, linebuf);
    for (int i = 0; i < yycolumn + 2; i++) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, "^\n");
}

%}

ALPHA [A-Za-z]
DIGIT [0-9]
ALNUM {ALPHA}|{DIGIT}

DEC   [1-9]{DIGIT}*
OCT   0[1-7][0-7]*
HEX   0[xX][0-9A-Fa-f]+

GEN_L_REG r((1[0-5])|([0-9]))
GEN_S_REG {GEN_L_REG}[abcd]
EXTRA_REG re[0-6]
KERN_REG  rk[0-7]
PTR_REG   r[srf]
REGISTER {GEN_L_REG}|{GEN_S_REG}|{EXTRA_REG}|{KERN_REG}|{FLAG_REG}|{PTR_REG}
 /* REGISTER r(((1[0-5])|([0-9]))[abcd]?|(e[0-6])|(k[0-7])|(flags|[srimpf])) */

%%

";"[^;\n]*$                 {/* comment */}

{ALPHA}{ALNUM}*:            { return TOK_LABEL; }
{GEN_L_REG}                 { return TOK_L_REG; }
{GEN_S_REG}                 { return TOK_S_REG; }
{EXTRA_REG}                 { return TOK_EXTRA_REG; }
{KERN_REG}                  { return TOK_KERN_REG; }
{PTR_REG}                   { return TOK_PTR_REG; }
 /* {REGISTER}                  { return TOK_REG; } */

{ALPHA}{ALNUM}*             { return TOK_WORD; }
[+-]?({HEX}|{DEC}|{OCT})    { return TOK_NUM; }

"+"                         { return TOK_PLUS; }
"-"                         { return TOK_MINUS; }
"["                         { return TOK_LBRACKET; }
"]"                         { return TOK_RBRACKET; }
","                         { return TOK_COMMA; }
"."                         { return TOK_DOT; }

[ \t]                       {/* ignore space */}
\n.*                        { lineno++; yycolumn = 1;
                              strncpy(linebuf, yytext+1, sizeof(linebuf));
                              yyless(1); /* give back all but the newline */
                              return TOK_NL; }

<<eof>>                     { return EOF; }

.                           { fprintf(stderr, "Unrecognized character %c\n",
                                      *yytext); }

%%

