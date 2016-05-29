#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#include "debug.h"
#include "JasParser.h"
#include "Instruction.h"
#include "Labels.h"
#include "Registers.h"

/** local fn prototypes **/
static void firstPass(void);
static void secondPass(void);

/* reading input */
static void readInstruction(void);
static void readLengthModifier(Instruction *);
static void readOperands(Instruction *, enum TokenType);
static void readOperand(Operand *, enum TokenType);
static int readNumber(void);
static void readRegister(Operand *, enum TokenType);

/* utility functions */
static OperandSize opSizeOfNum(int);
static int isRegister(enum TokenType);
static int isRegType(OperandType);

/* final output stream */
static FILE * ostream;

/* ------------------------ Main Entry Functions  --------------------------- */

/** entry function to begin parsing **/
void parse(FILE * in, FILE * out) {
    yyin = in;
    ostream = out;

    /* grab the first line */
    fgets(linebuf, MAX_LINE_LENGTH, in);
    linebuf[strlen(linebuf) - 1] = '\0';
    rewind(in);

    firstPass(); /* initial parsing, label recognition,
                    type saving and syntax checks */

    secondPass(); /* label resolution and type analysis */

    /* prevent writing to file if there were errors */
    if (!yyerr) {
        /* write instructions to outfile */
        for (int i = 0; i < numinstrs; i++) {
            writeInstruction(&instructions[i], ostream);
        }
    }
}

static void firstPass(void) {
    enum TokenType token;

    /* read line-by-line until end-of-input */
    while ((token = yylex()) != EOF) {

        /* skip empty lines */
        if (token == TOK_NL) continue;

        /* non-empty line: */
        /* two possibilities: label or instruction */
        if (token == TOK_LABEL) {

            saveLabel(yytext, lcounter);
            /* if newline after, just continue through */
            if (yylex() == TOK_NL) continue;
            else readInstruction();

        } else if (token == TOK_WORD) {

            readInstruction();

        } else {
            yyerror("Line must start with label or instruction.");
        }
    }
}

static void secondPass(void) {
    resolveLabels();
}

/* ----------------------- "Read" Functions --------------------------------- */

static void readLengthModifier(Instruction * instr) {
    yylex(); /* advance token */

    if (*yytext == 's') {
        instr->size = OPSZ_SHORT;
    } else if (*yytext == 'l') {
        instr->size = OPSZ_LONG;
    } else {
        ERR_QUIT("Invalid length modifier, expecting 's' or 'l'");
    }
}

static int readNumber(void) {
    long value;
    /* read in the number with strtol */
    errno = 0;
    value = strtol(yytext, NULL, ANY_BASE);
    if (errno != 0) {
        perror(NULL); /* FIXME: make a better error message */
        exit(1);
    }

    /* check for int size (we can support max of 32 bits) */
    if (value < INT_MIN || UINT_MAX < value) {
        yyerror("Integer larger than 32 bits.");
    }

    return (int) value;
}

static void readRegister(Operand * op, enum TokenType token) {
    /* check register size */
    if (token == TOK_S_REG) {
        op->size = OPSZ_SHORT;
    } else { /* the rest can have size long */
        op->size = OPSZ_LONG;
    }

    op->type = OT_REG;
    op->value = getRegisterId(yytext);
}

static void readOperands(Instruction * instr, enum TokenType token) {

    if (token != TOK_NL) {
        readOperand(&instr->op1, token);
    } else if (token == TOK_NL) {
        return; /* one-op */
    }

    token = yylex();
    if (token == TOK_COMMA) {
        readOperand(&instr->op2, yylex());
    } else if (token == TOK_NL) {
        /* no second operand */
        instr->op2.size = 0;
        return; /* two-op */
    } else {
        ERR_QUIT("Invalid operand.");
    }

}

static void readOperand(Operand * op, enum TokenType token) {

    /* four possibilities: constant, register, indirect, or word */
    if (token == TOK_NUM) {
        int value = readNumber();

        /* populate struct */
        op->type = OT_CONST;
        op->value = value;
        op->size = opSizeOfNum(value);
        /* .offset member irrelevant */

        lcounter++; /* constant takes up an extra word of space */

    } else if (isRegister(token)) {

        readRegister(op, token);

    } else if (token == TOK_LBRACKET) {

        token = yylex();

        /* two possibilities, register or number following */
        if (isRegister(token)) {

            readRegister(op, token);

            token = yylex();
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
                if (yylex() == TOK_NUM) {
                    offset = readNumber();
                    op->offset = sign * offset;
                    op->type = OT_REG_OFFSET;

                    /* next token must be a closing bracket */
                    token = yylex();
                    if (token != TOK_RBRACKET) {
                        ERR_QUIT("Expected ']'.");
                    }
                } else {
                    ERR_QUIT("Expected number for offset.");
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

            token = yylex();
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
    } else if (token == TOK_WORD) {
        /* assume it's a label, try to do label resolution
         * if we can't, it's fine, we'll get it on the second pass! */
        op->type = OT_CONST;
        op->size = OPSZ_LONG;
        op->value = getLabelLocation(yytext);

        lcounter++; /* label address takes up word of space */

        if (op->value == -1) {
            saveUndefLabel(yytext,
                           (char *)&op->value - (char *)instructions);
                           // FIXME: unclean
        }
    } else if (strlen(yytext) == 0) {
        /* no operands */
    } else {
        ERR_QUIT("Unrecognizable operand.");
    }

}

/* opcodes for CMP and TEST */
#define OP_CMP  0x05
#define OP_TEST 0x07

static void readInstruction(void) {
    enum TokenType token;
    lcounter++; /* count for a new instruction */

    /* if word, but not instruction */
    if (!isInstruction(yytext)) {
        ERR_QUIT("Line does not contain a valid instruction.");
        /* FIXME: account for possible .ascii declarations etc? */
    }

    Instruction * newInstr;
    newInstr = saveInstruction(yytext); /* create new instr entry */

    token = yylex(); /* two possibilities: length modifier or operand */

    if (token == TOK_DOT) {
        /* the next character must be a valid modifier */
        readLengthModifier(newInstr);
    }

    /* next lexeme must be an operand */
    readOperands(newInstr, token);

    /** special cases **/
    /* special case for CMP and TEST */
    if (newInstr->opcode == OP_CMP || newInstr->opcode == OP_TEST) {

        /* check the operand types and assign the prototype accordingly */
        if (isRegType(newInstr->op2.type))

            newInstr->type = IT_A;

        else if (isRegType(newInstr->op1.type)) {

            newInstr->type = IT_B;
            newInstr->opcode++; /* increase opcode to the B opcode */

        }
    }

    /* non-custom offsets: this is just to check if we need to increase location counter */
    if (hasCustomOffset(&newInstr->op1) || hasCustomOffset(&newInstr->op2)) {
        lcounter++;
    }

    /** semantic checks **/
    if (!instructionSizeAgreement(newInstr)) {
        ERR_QUIT("Instruction operands' sizes are not in agreement.");
    }

    if (!instructionTypeAgreement(newInstr)) {
        ERR_QUIT("Instruction operands do not agree with its prototype.");
    }
}


/* -------------------------- Utility Functions ----------------------------- */

static OperandSize opSizeOfNum(int value) {
    if (CHAR_MIN <= value && value <= CHAR_MAX)
        return OPSZ_SHORT; /* can fit into 8 bits */
    else
        return OPSZ_LONG;  /* fits into 32 bits */
}

static int isRegister(enum TokenType token) {
    return (token == TOK_L_REG || token == TOK_S_REG ||
            token == TOK_EXTRA_REG || token == TOK_KERN_REG ||
            token == TOK_PTR_REG);
}

static int isRegType(OperandType type) {
    return type == OT_REG ||
           type == OT_REG_ACCESS ||
           type == OT_REG_OFFSET;
}
