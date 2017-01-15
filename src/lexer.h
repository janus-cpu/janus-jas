#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>

/** enum for the token types to expect **/
typedef enum TokenType {

    TOK_L_REG,      /* register tokens */
    TOK_S_REG,
    TOK_EXTRA_REG,
    TOK_KERN_REG,
    TOK_PTR_REG,

    TOK_ID,       /* identifier, numbers */
    TOK_LABEL,
    TOK_INSTR,
    TOK_DATA_SEG,
    TOK_NUM,
    TOK_CHR_LIT,
    TOK_STR_LIT,

    TOK_PLUS,       /* operators */
    TOK_MINUS,
    TOK_DOT,

    TOK_LBRACKET,   /* delimiters */
    TOK_RBRACKET,
    TOK_COMMA,
    TOK_EOF = EOF,

    TOK_UNK

} TokenType;

extern int curr_line, curr_col;
extern char lexstr[];
extern FILE* lexfile;

/** lexer functions -------------------------------------------------------- **/

TokenType next_tok(void);

#endif
