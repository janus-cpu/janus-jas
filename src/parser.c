#include "parser.h"

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
static void parse(void);
static void analyze(void);

/* reading input */
static inline void parse_line(void);
static inline void parse_label(void);
static inline void parse_instruction(void);

static void parse_length_modifier(struct Instruction * instr);
static void parse_operands(struct Instruction * instr);
static void parse_operand(struct Operand * opnd);
static void parse_register(struct Operand * opnd);
static void parse_register_indirect(struct Operand * opnd);

static void readDataSegment(void);

/* utility functions */
static OperandSize opSizeOfNum(int);
static int isRegType(OperandType);

// Current token.
static TokenType token;

/* ------------------------ Main Entry Functions  --------------------------- */

/** entry function to begin assembling **/
void assemble(FILE * in, FILE * out) {
    // Let the lexer know about the infile.
    lexfile = in;

    // /* grab the first line */
    // fgets(linebuf, MAX_LINE_LENGTH, in);
    // linebuf[strlen(linebuf) - 1] = '\0';
    // rewind(in);

    parse(); /* initial parsing, label recognition,
                type saving and syntax checks */

    analyze(); /* label resolution and type analysis */

    /* prevent writing to file if there were errors */
    if (!j_err) {
        /* write instructions to outfile */
        writeInstructions(out);
    }
}

static void parse(void) {
    // Parse lines until EOF.
    while ((token = next_tok()) != TOK_EOF) {
        parse_line();
    }
}

static void analyze(void) {
    resolveLabels();
}

/* ------------------------- Parse Functions -------------------------------- */

/*
 * Each parse_* function expects that the current token loaded is of its type.
 * After it returns, the corresponding variable structure should be gone (e.g.
 * after parse_label() returns, the current token will have advanced past to
 * the token after the label).
 */

static inline void parse_line(void) {
    // Let by empty lines.
    if (token == TOK_NL) return;

    if (token == TOK_LABEL) {

        parse_label();

    } else if (token == TOK_INSTR) {

        parse_instruction();

    } else if (token == TOK_DATA_SEG) {

        readDataSegment();

    } else {
        jas_err("Line must start with label, instruction, or data segment.",
                curr_line, lo_col, curr_col);
    }
}

/*
 * Parse a label.
 *     e.g. `_L0:`
 * Pre-conditions: current token is TOK_LABEL.
 * Post-conditions: current token is the one following the label and its colon.
 */
static inline void parse_label(void) {
    saveLabel(lexstr, instrPtr);
}

/*
 * Parse a whole instruction (opcode and operands).
 * Pre-conditions: current token is TOK_INSTR.
 * Post-conditions: current token is the one following the last operand of the
 *                  instruction.
 */
static inline void parse_instruction(void) {
    DEBUG("Parsing instruction `%s'", lexstr);
    struct Instruction newInstr = {0};
    struct InstrRecord info;

    // Get instruction opcode.
    getInstrInfo(lexstr, &info);
    newInstr.name = info.name;
    newInstr.type = info.type;
    newInstr.opcode = info.opcode;

    // Parse next token.
    token = next_tok();

    // Length modifier?
    if (token == TOK_DOT) {
        // TODO write out this function and set below code into it
        parse_length_modifier(&newInstr);
    }

    // Read the operands.
    parse_operands(&newInstr);

    /* opcodes for CMP and TEST */
    #define OP_CMP  0x05
    #define OP_TEST 0x07

    // Special case for CMP and TEST, which have reverse operand types: A/B.
    if (newInstr.opcode == OP_CMP || newInstr.opcode == OP_TEST) {
        // Assign the appropriate prototype based on the operand types.
        if (isRegType(newInstr.op2.type)) {
            newInstr.type = IT_A;
        } else if (isRegType(newInstr.op1.type)) {
            newInstr.type = IT_B;
            newInstr.opcode++; // Set opcode to the next record (the B type).
        }
    }

    // FIXME: should these checks should be in the analyze() function?
    // Semantic checks
    DEBUG("  Semantic: checking `%s' with type %d\n\t  size1 %d size2 %d",
            newInstr.name,     newInstr.type,
            newInstr.op1.size, newInstr.op2.size);

    if (!instructionSizeAgreement(&newInstr)) {
        fprintf(stderr, "Instruction %s:\n", newInstr.name);
        ERR_QUIT("Instruction operands' sizes are not in agreement.");
    }

    if (!instructionTypeAgreement(&newInstr)) {
        ERR_QUIT("Instruction operands do not agree with its prototype.");
    }

    // XXX: is this a good place to write out the instruction?
    /* write the machine code for this instruction into the buffer */
    if (saveInstruction(&newInstr)) {
        ERR_QUIT("Could not write instruction!");
    }
}

/*
 * Parse the length modifier for an instruction.
 *     e.g. `.l` in `mov.l`
 * Pre-conditions: current token is TOK_DOT.
 * Post-conditions: current token is the one following the length modifier.
 *                  `instr` will contain size information of the instruction.
 */
static void parse_length_modifier(struct Instruction * instr) {
    DEBUG("  Forced length for `%s'.", instr->name);

    token = next_tok(); // Advance to length modifier.

    if (*lexstr == 's') {
        instr->size = OPSZ_SHORT;
    } else if (*lexstr == 'l') {
        instr->size = OPSZ_LONG;
    } else {
        ERR_QUIT("Invalid length modifier, expecting 's' or 'l'");
    }

    DEBUG("  Chose op size %d.", instr->size);

    token = next_tok(); // Advance to operands.
}

/*
 * Parse multiple operands for an instruction.
 * Pre-conditions: current token is /possibly/ the start of an operand.
 * Post-conditions: current token is one following the last operand.
 *                  `instr` will contain information about the operands.
 */
static inline void parse_operands(struct Instruction * instr) {
    // Because there are no-operand, one-operand, and two-operand instructions,
    // there is a possibility of TOK_NL appearing throughout parsing these.

    if (token == TOK_NL) return; // No-operand instruction.

    // First operand.
    parse_operand(&instr->op1);

    if (token == TOK_NL) return; // One-operand instruction.

    // Expect a comma before a second operand.
    if (token != TOK_COMMA) ERR_QUIT("Expected comma.");

    // Advance to next operand.
    token = next_tok();
    parse_operand(&instr->op2); // Two-operand instruction.
}

/*
 * Parse a single operand.
 * Pre-conditions: current token is /possibly/ the start of an operand.
 * Post-conditions: current token is one following the operand.
 *                  `opnd` will contain information about the operand.
 */
static void parse_operand(struct Operand * opnd) {
    DEBUG("  Reading operand starting with %s", lexstr);

    // Four possibilities: Constant, Register, Register Indirect, or Identifier.
    if (token == TOK_NUM) {

        // Populate struct with relevant info.
        opnd->type = OT_CONST;
        opnd->value = lexint;
        opnd->size = opSizeOfNum(lexint);
        // .offset member is irrelevant.

        // Advance token past the number.
        token = next_tok();

    } else if (isRegister(token)) {

        parse_register(opnd);

    } else if (token == TOK_LBRACKET) {

        parse_register_indirect(opnd);

    } else if (token == TOK_ID) {
        // Assume it's a label, try to do label resolution.
        // If we can't, it's fine, we'll get it on the second pass!
        opnd->type = OT_CONST;
        opnd->size = OPSZ_LONG;
        opnd->value = getLabelLocation(lexstr);

        // Save undefined labels, increment LC to next available space.
        if (opnd->value == -1) {
            saveUndefLabel(lexstr, instrPtr + sizeof(int));
        }

        // Advance token past the identifier.
        token = next_tok();

    } else {
        ERR_QUIT("Unrecognizable operand.");
    }
}

/*
 * Parses a register.
 *     e.g. `r0`, `r0a`, `rk0`
 * Pre-conditions: current token is one of TOK_GL_REG, TOK_GS_REG, TOK_E_REG,
 *                 or TOK_K_REG.
 * Post-conditions: current token is the one following the register token.
 *
 */
static void parse_register(struct Operand * opnd) {
    /* check register size */
    if (token == TOK_GS_REG) {
        opnd->size = OPSZ_SHORT;
    } else { /* the rest can have size long */
        opnd->size = OPSZ_LONG;
    }

    opnd->type = OT_REG;
    opnd->value = getRegisterId(lexstr);

    // Advance to token after register.
    token = next_tok();
}

/*
 * Parses an indirect memory access, with an offset or not.
 *     e.g. `[r0]` or `[r0 + 4]` or `[-4 + r0]`
 * Pre-conditions: current token is TOK_LBRACKET.
 * Post-conditions: current token is after the TOK_RBRACKET of the indirect.
 *                  `opnd` will contain information about the operand.
 */
static void parse_register_indirect(struct Operand * opnd) {
    token = next_tok(); // Grab inner token of the indirection.

    // Two possibilities, Register or Register with Constant following.
    if (!isRegister(token) && token != TOK_NUM)
        ERR_QUIT("Expected register or number following '['.");

    if (isRegister(token)) {
        // Get that register.
        parse_register(opnd);

        if (token == TOK_RBRACKET) {

            opnd->type = OT_REG_ACCESS; // Set type to simple indirect access.

        } else if (token == TOK_PLUS
                || token == TOK_MINUS
                || token == TOK_NUM) {
            // Save the sign of the offset.
            int sign = (token == TOK_PLUS ? +1 : -1);
            int offset; // Hold offset 

            // If we accidentally parse a TOK_NUM here but
            // it has [+-] as its start character, use it as the offset.
            if (token == TOK_NUM && (*lexstr == '-' || *lexstr == '+')) {
                offset = lexint;
            } else {
                // Next token should be a number.
                token = next_tok();
                if (token != TOK_NUM) ERR_QUIT("Expected constant offset.");
                offset = sign * lexint;
            }

            opnd->offset = offset;
            opnd->type = OT_REG_OFFSET;

            // Advance token to ']'.
            token = next_tok();
        } else {
            ERR_QUIT("Expected '+', '-', or ']'.");
        }

    } else if (token == TOK_NUM) {

        // Set data as offset indirect access.
        opnd->type = OT_REG_OFFSET;
        opnd->offset = lexint;

        token = next_tok();
        // Next token should be a plus sign.
        if (token != TOK_PLUS)
            ERR_QUIT("Expected '+'.");

        // Next token should be a register.
        parse_register(opnd);
    }

    // Next token must be a closing bracket.
    if (token != TOK_RBRACKET) ERR_QUIT("Expected ']'.");

    // Advance to token after ']'.
    token = next_tok();
}

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
                if (token == TOK_NUM || token == TOK_CHR_LIT) {

                    /* read number, check range */
                    byte = lexint;
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
                    jas_err("Expected byte value.", curr_line,
                            lo_col, curr_col);
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
                    word = lexint;

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
