#ifndef PARSER_H
#define PARSER_H

/* probably the maximum line length */
#define MAX_LINE_LENGTH 500

/* error string: gets printed out for every error */
#define ERROR_STRING "\033[1m%s:%d: \033[31merror\033[39m: %s\n" \
                     ">>>>\033[0m%s\n"

/** enum for the token types to expect **/
enum TokenType {

    TOK_LABEL,      /* basically TOK_WORD followed by a colon */
    /*TOK_REG,*/

    TOK_L_REG,      /* register tokens */
    TOK_S_REG,
    TOK_EXTRA_REG,
    TOK_KERN_REG,
    TOK_PTR_REG,

    TOK_WORD,       /* identifier, numbers */
    TOK_NUM,

    TOK_PLUS,       /* operators */
    TOK_MINUS,
    TOK_DOT,

    TOK_LBRACKET,   /* delimiters */
    TOK_RBRACKET,
    TOK_COMMA,
    TOK_NL,

    TOK_OTHER = -2  /* FIXME: is this really necessary? */
};

/** function prototypes **/
void parse(FILE * in, FILE * out);

#endif
