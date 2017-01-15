#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

/** enum for the token types to expect **/
typedef enum TokenType {

    /* register tokens */
    TOK_GL_REG,    // General Large
    TOK_GS_REG,    // General Small
    TOK_E_REG,     // Extra
    TOK_K_REG,     // Kernel

    /* identifiers, numbers */
    TOK_ID,
    TOK_LABEL,
    TOK_INSTR,
    TOK_DATA_SEG, // TODO -> TOK_DTV for directives
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

    TOK_NL,
    TOK_EOF = EOF,
    TOK_UNK

} TokenType;

extern int j_err;
extern int curr_line, curr_col, lo_col;
extern char lexstr[];
extern FILE* lexfile;

/** lexer functions -------------------------------------------------------- **/

void jas_err(const char* msg, int line, int lo, int hi);
TokenType next_tok(void);

#endif
