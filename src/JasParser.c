#include "JasParser.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "lexer.h"
#include "debug.h"
#include "Instruction.h"
#include "Labels.h"
#include "Registers.h"

/** local fn prototypes **/
static void firstPass(void);
static void secondPass(void);

/* reading input */
static void readDataSegment(void);
static void readInstruction(void);
static void readLengthModifier(Instruction *);
static void readOperands(Instruction *, TokenType);
static void readOperand(Operand *, TokenType);
static int readNumber(void);
static void readRegister(Operand *, TokenType);

/* utility functions */
static OperandSize opSizeOfNum(int);
static int isRegType(OperandType);

/* final output stream */
static FILE * ostream;

/* ------------------------ Main Entry Functions  --------------------------- */

/** entry function to begin parsing **/
void parse(FILE * in, FILE * out) {
    lexfile = in;
    ostream = out;

    /* grab the first line */
    fgets(linebuf, MAX_LINE_LENGTH, in);
    linebuf[strlen(linebuf) - 1] = '\0';
    rewind(in);

    firstPass(); /* initial parsing, label recognition,
                    type saving and syntax checks */

    secondPass(); /* label resolution and type analysis */

    /* prevent writing to file if there were errors */
    if (!j_err) {
        /* write instructions to outfile */
        writeInstructions(out);
    }
}

static void firstPass(void) {
    TokenType token;

    // TODO Integrate lexer with parser
    /* read line-by-line until end-of-input */
    while ((token = next_tok()) != TOK_EOF) {

        /* non-empty line: */
        /* two possibilities: label or instruction */
        if (token == TOK_LABEL) {

            saveLabel(lexstr, instrPtr);

        } else if (token == TOK_INSTR) {

            readInstruction();

        } else if (token == TOK_DATA_SEG) {

            readDataSegment();

        } else {
            jas_err("Line must start with label, instruction, or data segment.",
                    curr_line, lo_col, curr_col);
        }
    }
}

static void secondPass(void) {
    resolveLabels();
}

/* ----------------------- "Read" Functions --------------------------------- */

static void readDataSegment(void) {
    TokenType token;
    char * temp; /* reallocation temp */

    DEBUG("Data segment `%s'", lexstr);

    /* what kind of segment is it? */
    switch (lexstr[strlen(lexstr)-1]) {
        case 's': {
            token = next_tok();
            if (token == TOK_STR_LIT) {
                char * lptr = lexstr;
                char letter;

                DEBUG("  Reading string `%s'", lexstr);

                /* reallocate space for string */
                instrCap = instrPtr + strlen(lexstr);
                temp = (char *) realloc(instrBuffer, instrCap);
                if (temp == NULL) {
                    fprintf(stderr, "realloc() error.\n");
                    exit(1);
                }
                instrBuffer = temp;

                while (*lptr != '\0') {
                    letter = *lptr;

                    /* handle escaped characters */
                    if (letter == '\\') {
                        switch (*(++lptr)) {
                            case '0': letter = '\0'; break;
                            case 'n': letter = '\n'; break;
                            case 't': letter = '\t'; break;
                        }
                    }

                    /* save character into the buffer */
                    instrBuffer[instrPtr++] = letter;
                    lptr++;
                }
            } else {
                jas_err("Expected string.", curr_line, lo_col, curr_col);
            }
            break;
        }

        case 'b': {
            /* read in list of numbers/char literals as 8-bit integers */
            int byte;

            while ((token = next_tok()) != TOK_NL) {
                /* 8-bit integer should come first */
                if (token == TOK_NUM) {

                    /* read number, check range */
                    byte = readNumber();
                    if (byte < SCHAR_MIN || SCHAR_MAX < byte)
                        jas_err("Number too large to fit in 8-bits.",
                                  curr_line, lo_col, curr_col);


                    /* reallocate space for byte */
                    instrCap = instrPtr + 1;
                    temp = (char *) realloc(instrBuffer, instrCap);
                    if (temp == NULL) {
                        fprintf(stderr, "realloc() error.\n");
                        exit(1);
                    }
                    instrBuffer = temp;

                    /* write byte to buffer */
                    instrBuffer[instrPtr++] = (signed char) byte;

                } else {
                    jas_err("Expected number.", curr_line, lo_col, curr_col);
                }

                /* comma or newline should follow */
                token = next_tok();
                if (token != TOK_COMMA && token != TOK_NL)
                    jas_err("Expected `,'.", curr_line, lo_col, curr_col);

                if (token == TOK_NL) return; /* we're done, eol */
            }

        }

        case 'w': {
            /* read in the list of numbers as 32-bit integers */
            int word;

            while ((token = next_tok()) != TOK_NL) {
                /* 32-bit integer should come first */
                if (token == TOK_NUM) {

                    /* read the word in */
                    word = readNumber();

                    /* reallocate space for word */
                    instrCap = instrPtr + sizeof(word);
                    temp = (char *) realloc(instrBuffer, instrCap);
                    if (temp == NULL) {
                        fprintf(stderr, "realloc() error.\n");
                        exit(1);
                    }
                    instrBuffer = temp;

                    /* write word to buffer */
                    memcpy(instrBuffer + instrPtr, &word, sizeof(word));
                    instrPtr += sizeof(word);

                } else {
                    jas_err("Expected number.", curr_line, lo_col, curr_col);
                }

                /* comma or newline should follow */
                token = next_tok();
                if (token != TOK_COMMA && token != TOK_NL)
                    jas_err("Expected `,'.", curr_line, lo_col, curr_col);

                if (token == TOK_NL) return; /* we're done, eol */
            }
        }

        default:
            jas_err("Non-existent data segment type.", curr_line, lo_col,
                      curr_col);
    }

}

static void readLengthModifier(Instruction * instr) {
    next_tok(); /* advance token */

    if (*lexstr == 's') {
        instr->size = OPSZ_SHORT;
    } else if (*lexstr == 'l') {
        instr->size = OPSZ_LONG;
    } else {
        ERR_QUIT("Invalid length modifier, expecting 's' or 'l'");
    }
}

static int readNumber(void) {
    long value;

    DEBUG("  Reading number %s", lexstr);

    /* read in the number with strtol */
    errno = 0;
    value = strtol(lexstr, NULL, ANY_BASE);
    if (errno != 0) {
        perror(NULL); /* FIXME: make a better error message */
        exit(1);
    }

    /* check for int size (we can support max of 32 bits) */
    if (value < INT_MIN || UINT_MAX < value) {
        jas_err("Integer larger than 32 bits.", curr_line, lo_col, curr_col);
    }

    return (int) value;
}

static void readRegister(Operand * op, TokenType token) {
    /* check register size */
    if (token == TOK_GS_REG) {
        op->size = OPSZ_SHORT;
    } else { /* the rest can have size long */
        op->size = OPSZ_LONG;
    }

    op->type = OT_REG;
    op->value = getRegisterId(lexstr);
}

static void readOperands(Instruction * instr, TokenType token) {

    if (token != TOK_NL) {
        readOperand(&instr->op1, token);
    } else if (token == TOK_NL) {
        return; /* one-op */
    }

    token = next_tok();
    if (token == TOK_COMMA) {
        readOperand(&instr->op2, next_tok());
    } else if (token == TOK_NL) {
        /* no second operand */
        instr->op2.size = 0;
        return; /* two-op */
    } else {
        ERR_QUIT("Invalid operand.");
    }

}

static void readOperand(Operand * op, TokenType token) {

    DEBUG("  Reading operand starting with %s", lexstr);

    /* four possibilities: constant, register, indirect, or word */
    if (token == TOK_NUM) {
        int value = readNumber();

        /* populate struct */
        op->type = OT_CONST;
        op->value = value;
        op->size = opSizeOfNum(value);
        /* .offset member irrelevant */

    } else if (isRegister(token)) {

        readRegister(op, token);

    } else if (token == TOK_LBRACKET) {

        token = next_tok();

        /* two possibilities, register or number following */
        if (isRegister(token)) {

            readRegister(op, token);

            token = next_tok();
            /* two possibilities: closing bracket or a +/- sign */
            if (token == TOK_RBRACKET) {
                /* this is a simple reg access, correct the type and
                 * send back*/
                op->type = OT_REG_ACCESS;

            } else if (token == TOK_PLUS || token == TOK_MINUS) {
                /* keep track of the sign */
                int sign = (token == TOK_PLUS ? +1 : -1);
                int offset;

                /* next token must be a number */
                if (next_tok() == TOK_NUM) {
                    offset = readNumber();
                    op->offset = sign * offset;
                    op->type = OT_REG_OFFSET;

                    /* next token must be a closing bracket */
                    token = next_tok();
                    if (token != TOK_RBRACKET) {
                        ERR_QUIT("Expected ']'.");
                    }
                } else {
                    ERR_QUIT("Expected number for offset.");
                }
            } else if (token == TOK_NUM) {
              int offset;
              // If we accidentally parse a number, but the number
              // matches the regex [+-][0-9]+, then we have a correct
              // offset actually.
              if (lexstr[0] == '+' || lexstr[0] == '-') {
                offset = readNumber();
                op->offset = offset;
                op->type = OT_REG_OFFSET;
              } else {
                ERR_QUIT("Expected '+' or '-' before number.");
              }

              /* next token must be a closing bracket */
              token = next_tok();
              if (token != TOK_RBRACKET) {
                  ERR_QUIT("Expected ']'.");
              }
            } else {
                ERR_QUIT("Expected '+', '-', or ']'.");
            }

        } else if (token == TOK_NUM) {
            int offset;

            /* next token must be a number */
            offset = readNumber();

            /* can assume this is an indirect offset access */
            op->type = OT_REG_OFFSET;
            op->offset = offset;

            token = next_tok();
            /* next token should be a plus sign */
            if (token == TOK_PLUS) {
                /* next token should be a register */
                readRegister(op, token);
            } else {
                ERR_QUIT("Expected '+'.");
            }
        } else {
            ERR_QUIT("Expected register or number following '['.");
        }
    } else if (token == TOK_ID) {
        /* assume it's a label, try to do label resolution
         * if we can't, it's fine, we'll get it on the second pass! */
        op->type = OT_CONST;
        op->size = OPSZ_LONG;
        op->value = getLabelLocation(lexstr);

        /* save undefined labels, increment to next avail space */
        if (op->value == -1) {
            saveUndefLabel(lexstr, instrPtr + sizeof(int));
        }
    } else if (strlen(lexstr) == 0) {
        /* no operands */
    } else {
        ERR_QUIT("Unrecognizable operand.");
    }

}

/* opcodes for CMP and TEST */
#define OP_CMP  0x05
#define OP_TEST 0x07

static void readInstruction(void) {
    TokenType token;

    DEBUG("Reading instruction `%s'", lexstr);

    Instruction newInstr = {0};
    InstrRecord info;

    /* get opcode for the instruction */
    getInstrInfo(lexstr, &info);
    newInstr.name = info.name;
    newInstr.type = info.type;
    newInstr.opcode = info.opcode;

    token = next_tok(); /* two possibilities: length modifier or operand */

    if (token == TOK_DOT) {
        DEBUG("  Forced length for `%s'.", newInstr.name);
        /* the next character must be a valid modifier */
        readLengthModifier(&newInstr);
        token = next_tok(); /* advance token */
    }

    /* next lexeme must be an operand */
    readOperands(&newInstr, token);

    /** special cases **/
    /* special case for CMP and TEST */
    if (newInstr.opcode == OP_CMP || newInstr.opcode == OP_TEST) {

        /* check the operand types and assign the prototype accordingly */
        if (isRegType(newInstr.op2.type))

            newInstr.type = IT_A;

        else if (isRegType(newInstr.op1.type)) {

            newInstr.type = IT_B;
            newInstr.opcode++; /* increase opcode to the B opcode */

        }
    }

    /** semantic checks **/

    DEBUG("  Semantic: checking `%s' with type %d\n\t  op1 %d op2 %d",
            newInstr.name,     newInstr.type,
            newInstr.op1.size, newInstr.op2.size);

    if (!instructionSizeAgreement(&newInstr)) {
        ERR_QUIT("Instruction operands' sizes are not in agreement.");
    }

    if (!instructionTypeAgreement(&newInstr)) {
        ERR_QUIT("Instruction operands do not agree with its prototype.");
    }

    /* write the machine code for this instruction into the buffer */
    if (saveInstruction(&newInstr)) {
        ERR_QUIT("Could not write instruction!");
    }
}


/* -------------------------- Utility Functions ----------------------------- */

static OperandSize opSizeOfNum(int value) {
    if (CHAR_MIN <= value && value <= CHAR_MAX)
        return OPSZ_SHORT; /* can fit into 8 bits */
    else
        return OPSZ_LONG;  /* fits into 32 bits */
}

int isRegister(TokenType token) {
    return (token == TOK_GL_REG || token == TOK_GS_REG ||
            token == TOK_E_REG || token == TOK_K_REG);
}

static int isRegType(OperandType type) {
    return type == OT_REG ||
           type == OT_REG_ACCESS ||
           type == OT_REG_OFFSET;
}
