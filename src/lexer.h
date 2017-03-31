#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

/** enum for the token types to expect **/
typedef enum TokenType {

    /* register tokens */
    TOK_GL_REG,    // General Large
    TOK_GS_REG,    // General Small

    /* identifiers, numbers */
    TOK_ID,
    TOK_LABEL,
    TOK_INSTR,
    TOK_NUM,
    TOK_CHR_LIT,
    TOK_STR_LIT,

    /* operators */
    TOK_PLUS,
    TOK_MINUS,
    TOK_DOT,

    /* delimiters */
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_COMMA,

    /* directives */
    TOK_DATA_BYTE,
    TOK_DATA_HALF,
    TOK_DATA_WORD,
    TOK_DATA_STR,

    TOK_NL,
    TOK_EOF = EOF,
    TOK_UNK

} TokenType;

extern int j_err;
extern int curr_line, curr_col, lo_col;
extern char lexstr[];
extern long lexint;
extern FILE* lexfile;

/** lexer functions -------------------------------------------------------- **/

void jas_err(const char* msg, int line, int lo, int hi);
TokenType next_tok(void);
int is_register(TokenType token);
int is_directive(TokenType token);

#endif
