#ifndef PARSER_H
#define PARSER_H

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
