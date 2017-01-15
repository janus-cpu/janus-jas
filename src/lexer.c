#include "lexer.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "Instruction.h"

#define ERROR_FMT "(%d:%d) -> \033[1;31merror\033[0m: %s\n"

FILE* lexfile;
int curr_line = 1, curr_col = 0, last_col; // TODO last_col necessary?for spit()
int lo_col = 0; // The column at the beginning of a token.

static int curr_char;
char lexstr[BUFSIZ];
long lexint;

/*
 * Get a line from a FILE* stream. Does not include \n, but is null terminated.
 */
static char* fgetline(char buf[], FILE* stream) {
    char c;
    int i = 0;
    while ((c = fgetc(stream)) != '\n' && c != EOF)
        buf[i++] = c;
    buf[i] = '\0';
}

/*
 * Print a `^` at a given column from the left of the screen.
 */
static void fprint_caret(FILE* stream, int lo, int hi) {
    fputc('\t', stream);

    int col = 1;
    while (col < lo) {
        fputc(' ', stream);
        col++;
    }

    fprintf(stream, "\033[1;33m");
    fputc('^', stream);
    while (col < hi) {
        fputc('~', stream);
        col++;
    }
    fprintf(stream, "\033[0m");
    fputc('\n', stream);
}

/*
 * Error-reporting function. Provides message and relevant code snippet to user.
 */
static void lex_err(const char* msg, int line, int lo, int hi) {
    char linestr[BUFSIZ];
    long int pos;
    fprintf(stderr, ERROR_FMT, line, hi, msg);

    // Save position and seek back to the front of the line.
    // We need to save the absolute position, since getting the whole line later
    // moves the file ptr an unknown distance.
    pos = ftell(lexfile);
    fseek(lexfile, -hi, SEEK_CUR);

    // Print out whole line with caret.
    fgetline(linestr, lexfile);
    fprintf(stderr, "\t%s\n", linestr);
    fprint_caret(stderr, lo, hi);

    // Move back.
    fseek(lexfile, pos, SEEK_SET);
}

/*
 * Grabs the next character from the lexfile, 'eating' the current one.
 * Side effects: modifies curr_char, advances lexfile, increments the line and
 *               col counters.
 */
static inline int eat(void) {
    if (curr_char == '\n') {
        curr_line++;
        last_col = curr_col; // FIXME: do we need last_col?
        curr_col = 0;
    }
    curr_col++;
    return curr_char = fgetc(lexfile);
}

/*
 * Peek at next character without eating current character.
 */
static inline int peek(void) {
    int c = fgetc(lexfile);
    ungetc(c, lexfile);
    return c;
}

/*
 * Spits a character back up. Helper function for reversing input.
 * Side effects: backtracks lexfile, decrements the line and col counters.
 */
static inline void spit(char c) {
    // Don't spit when you haven't eaten.
    if (curr_line == 1 && curr_col == 1) return;

    // Restore line and col numbers.
    if (c == '\n') {
        curr_line--;
        curr_col = last_col + 1;
    }
    ungetc(c, lexfile);
    curr_col--;
}

/** helper functions -------------------------------------------------------- */
static inline int is_idstart(int c) {
    return isalpha(c) || c == '$' || c == '_';
}

static inline int is_idcont(int c) {
    return isalnum(c) || c == '$' || c == '_';
}

static inline int is_oct(int c) {
    return '0' <= c && c <= '7';
}

static inline int is_bin(int c) {
    return c == '0' || c == '1';
}

static inline char escape(char c) {
    switch (c) {
        case 't': return '\t';
        case 'n': return '\n';
        case 'r': return '\r';
        case 'b': return '\b';
        case 'f': return '\f';
        case 'v': return '\v';
        case '0': return '\0';
        default: return c;
    }
}

/*
 * Read in characters for a number in a certain base. Assumes curr_char is on
 * the first character of the numeric literal.
 * Side effects: modifies curr_char.
 */
static int fgets_base(char buf[], FILE* stream, int base) {
    int i = 0, extra = 0;
    if (base == 2 || base == 16) {
        fgetc(stream);
        curr_char = fgetc(stream); // Move past 0x / 0b.
        extra = 2;
    }

    int (*check)(int); // Condition each digit must fulfill.

    switch (base) {
        case 2: check = is_bin; break;
        case 8: check = is_oct; break;
        case 10: check = isdigit; break;
        case 16: check = isxdigit; break;
    }

    while (curr_char != EOF && check(curr_char)) {
        buf[i++] = curr_char;
        curr_char = fgetc(stream);
    }

    buf[i] = '\0';

    return extra + i;
}

/** lexer ------------------------------------------------------------------- */

/*
 * Gets the next token from the `lexfile` FILE stream.
 * Side effects:
 *      - If the TokenType has an associated string, it is found in `lexstr`.
 *      - If the TokenType has an associated integer value, look in `lexint`.
 */
TokenType next_tok(void) {
    if (!curr_char) eat(); // Eat first char.

    while (curr_char != EOF) {
        lo_col = curr_col; // Save first col of the token.

        // Skip whitespace.
        if (isspace(curr_char)) {
            eat();
            continue;
        }

        // Skip comments until next line.
        if (curr_char == ';') {
            do {
                eat();
            } while (curr_char != '\n' && curr_char != EOF);
            continue;
        }

        // id ::= [A-Za-z$_][A-Za-z_$0-9]*
        // label ::= <nonopcode id>:
        if (is_idstart(curr_char)) {
            int i;
            // TODO: Make this loop prettier.
            for (i = 0; curr_char != EOF; i++) {
                lexstr[i] = curr_char;
                if (is_idcont(peek())) {
                    eat();
                } else {
                    break;
                }
            }
            lexstr[++i] = '\0';
            eat(); // Advance to next char.

            // Instruction?
            if (isInstruction(lexstr))
                return TOK_INSTR;

            // Label?
            if (curr_char == ':') {
                eat(); // Eat the ':'
                return TOK_LABEL;
            }

            // Plain identfier
            return TOK_ID;
        }

        // chr_lit ::= '[^\\']'
        if (curr_char == '\'') {
            eat(); // Get inner char.
            if (curr_char == '\\') {
                lexstr[0] = escape(eat());
            } else {
                lexstr[0] = curr_char;
            }
            lexstr[1] = '\0';
            eat(); // Reach the closing quote.

            // Error: for situations like '\'
            if (curr_char != '\'') {
                lex_err("Character literal missing closing quote.",
                         curr_line, lo_col, curr_col);
                return TOK_UNK;
            }

            eat(); // Get rid of ' and advance.
            return TOK_CHR_LIT;
        }

        // str_lit ::= "(\\.|[^\\"])*"
        if (curr_char == '"') {
            eat(); // Get first char of string.

            // Let by escape chars, but not single \ or ".
            int i;
            for (i = 0; curr_char != '"'; i++) {
                // Check that we don't close reach EOF before the close ".
                if (curr_char == EOF) {
                    lex_err("EOF while parsing string literal.",
                            curr_line, lo_col, curr_col);
                    return TOK_UNK;
                }

                if (curr_char == '\\') {
                    lexstr[i] = escape(eat());
                } else {
                    lexstr[i] = curr_char;
                }
                eat();
            }
            lexstr[i] = '\0';

            eat(); // Get rid of the " and advance.
            return TOK_STR_LIT;
        }

        // num_lit ::= [1-9][0-9]* | 0[0-7]* | 0x[0-9A-Fa-f]+ | 0b[01]+
        if (isdigit(curr_char)) {
            int base = 10;  // Numeric base for interpreting the literal.
            int chars_read; // Keep track of how many columns we move forward.

            // Choose base by prefix:
            if (curr_char == '0') {
                int next = peek();
                if (next == 'x' || next == 'X') {
                    base = 16;
                } else if (next == 'b' || next == 'B') {
                    base = 2;
                } else {
                    base = 8;
                }
            }

            // Copy in whole num literal, keep track of columns.
            chars_read = fgets_base(lexstr, lexfile, base);
            curr_col += chars_read;

            // Convert to integer value.
            lexint = strtol(lexstr, NULL, base);
            return TOK_NUM;
        }

        // Let by various punctuation:
        switch (curr_char) {
            case ',': eat(); return TOK_COMMA;
            case '.': eat(); return TOK_DOT;
            case '+': eat(); return TOK_PLUS;
            case '-': eat(); return TOK_MINUS;
            case '[': eat(); return TOK_LBRACKET;
            case ']': eat(); return TOK_RBRACKET;
        }

        lex_err("Unknown character encountered.", curr_line, lo_col, lo_col);
        eat(); // Advance to next char.
    }

    return TOK_EOF;
}

// TODO remove
#include <stdbool.h>
bool debug_on = false;

int main(int argc, char* argv[]) {
    FILE* f = fopen(argv[1], "r");
    if (f) {
        TokenType tty;
        lexfile = f;
        while ((tty = next_tok()) != TOK_EOF) {
            switch (tty) {
                case TOK_INSTR:
                    fprintf(stderr, "INSTR `%s`\n", lexstr);
                    break;
                case TOK_ID:
                    fprintf(stderr, "ID `%s`\n", lexstr);
                    break;
                case TOK_NUM:
                    fprintf(stderr, "NUM `%d`\n", lexint);
                    break;
            }
        }
        fclose(f);
    }
}
