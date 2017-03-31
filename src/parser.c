#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"
#include "debug.h"
#include "instruction.h"
#include "labels.h"
#include "registers.h"
#include "util.h"
#include "output.h"
#include "jas_limits.h"

/** local fn prototypes **/

/* reading input */
static inline void parse_line(void);
static inline void parse_label(void);
static inline void parse_instruction(void);
static inline void parse_directive(void);

static void parse_length_modifier(struct Instruction * instr);
static void parse_operands(struct Instruction * instr);
static void parse_operand(struct Operand * opnd);
static void parse_register(struct Operand * opnd);
static void parse_register_indirect(struct Operand * opnd);
static void parse_data_str(void);
static void parse_data_byte(void);
static void parse_data_half(void);
static void parse_data_word(void);

// Current token.
static TokenType token;

/* ------------------------ Main Entry Functions  --------------------------- */

void parse(void) {
    // Parse lines until EOF.
    while ((token = next_tok()) != TOK_EOF) {
        parse_line();
    }
}

void analyze(void) {
    resolve_labels();
}

/* ------------------------- Parse Functions -------------------------------- */

/*
 * Each parse_* function expects that the current token loaded is of its type.
 * After it returns, the corresponding variable structure should be gone (e.g.
 * after parse_label() returns, the current token will have advanced past to
 * the token after the label).
 */

/*
 * Parse a line.
 * Pre-conditions: current token is a beginning-of-line token:
 *  - TOK_NL, TOK_LABEL, TOK_INST, TOK_DATA_SEG
 * Post-conditions: current token is TOK_NL.
 */
static inline void parse_line(void) {
    // Let by empty lines.
    if (token == TOK_NL) return;

    if (token == TOK_LABEL) {

        parse_label();

    } else if (token == TOK_INSTR) {

        parse_instruction();

    } else if (is_directive(token)) {
        // TODO: Extends to non-data directives later
        parse_directive();

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
    save_label(lexstr);
}

/*
 * Parse a whole instruction (opcode and operands).
 * Pre-conditions: current token is TOK_INSTR.
 * Post-conditions: current token is the one following the last operand of the
 *                  instruction.
 */
static inline void parse_instruction(void) {
    DEBUG(">> Parsing instruction `%s`", lexstr);
    struct Instruction newInstr = {0};
    struct InstrRecord info = {0};
    char instr_name[10]; // HACK: Save instruction name for synthetic instrs

    // Save instruction line/col info for error messages.
    int instr_line = curr_line;
    int instr_lo_col = lo_col;
    int instr_curr_col = curr_col;

    // Save instruction name in all caps.
    strcpyup(instr_name, lexstr);

    // Get instruction opcode.
    instr_info(lexstr, &info);
    newInstr.name = instr_name;
    newInstr.type = info.type;
    newInstr.opcode = info.opcode;

    // Parse next token.
    token = next_tok();

    // Length modifier?
    if (token == TOK_DOT) {
        parse_length_modifier(&newInstr);
    }

    // Read the operands.
    parse_operands(&newInstr);

    // Check that the operand types agree with the instruction.
    if (!instructionTypeAgreement(&newInstr)) {
        jas_err("Instruction operands do not agree with its prototype.",
                instr_line, instr_lo_col, instr_curr_col);
        return;
    }

    // If instruction is synthetic, turn it into its aliased instruction.
    // XXX: What about synthetic instructions that expand into more than one?
    unalias_instruction(&newInstr);

    // FIXME: should these checks should be in the analyze() function?
    // Semantic checks
    DEBUG(" \\ Semantic: checking `%s` ty=%d sz=%d sz1=%d sz2=%d",
            newInstr.name,     newInstr.type, newInstr.size,
            newInstr.op1.size, newInstr.op2.size);

    if (!instructionSizeAgreement(&newInstr)) {
        jas_err("Instruction operands' sizes are not in agreement.",
                instr_line, instr_lo_col, instr_curr_col);
        return;
    }

    // XXX: is this a good place to write out the instruction?
    /* write the machine code for this instruction into the buffer */
    if (save_instruction(&newInstr)) {
        jas_err("Could not write instruction!",
                instr_line, instr_lo_col, instr_curr_col);
        return;
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
    token = next_tok(); // Advance to length modifier.

    if (*lexstr == 's') {
        instr->size = OS_SHORT;
    } else if (*lexstr == 'l') {
        instr->size = OS_LONG;
    } else {
        ERR_QUIT("Invalid length modifier, expecting 's' or 'l'");
    }

    DEBUG(" | Forced op size %d.", instr->size);

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

    // Check consistency with instruction type.
    if (!has_operands(instr->type))
        ERR_QUIT("Instruction doesn't have operands.");

    DEBUG(" - Reading first operand%s","");

    // First operand.
    parse_operand(&instr->op1);

    if (token == TOK_NL) return; // One-operand instruction.

    // Check consistency with instruction type.
    if (!has_two_operands(instr->type))
        ERR_QUIT("Instruction doesn't have two operands.");

    // Expect a comma before a second operand.
    if (token != TOK_COMMA) ERR_QUIT("Expected comma.");

    DEBUG(" - Reading second operand%s","");

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

    // Four possibilities: Constant, Register, Register Indirect, or Identifier.
    if (token == TOK_NUM) {
        DEBUG(" | Reading number `%s`", lexstr);

        // Populate struct with relevant info.
        opnd->type = OT_CONST;
        opnd->const_size = fit_size(lexint);
        opnd->size = opnd->const_size > 1 ? OS_LONG : OS_SHORT;
        opnd->constant = lexint;

        // Advance token past the number.
        token = next_tok();

    } else if (is_register(token)) {

        parse_register(opnd);

    } else if (token == TOK_LBRACKET) {

        parse_register_indirect(opnd);

    } else if (token == TOK_ID) {
        DEBUG(" | Reading identifier `%s`", lexstr);

        // Assume it's a label, try to do label resolution.
        // If we can't, it's fine, we'll get it on the second pass!
        opnd->type = OT_CONST;
        opnd->const_size = CS_WORD;
        opnd->size = OS_LONG;
        opnd->constant = label_address(lexstr);

        // Save undefined labels.
        if (opnd->constant == -1) {
            save_undef_label(lexstr);
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
    DEBUG(" | Reading register `%s`", lexstr);
    /* check register size */
    if (token == TOK_GS_REG) {
        opnd->size = OS_SHORT;
    } else { /* the rest can have size long */
        opnd->size = OS_LONG;
    }

    opnd->type = OT_REG;
    opnd->reg = register_id(lexstr);

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
    if (!is_register(token) && token != TOK_NUM)
        ERR_QUIT("Expected register or number following '['.");

    if (is_register(token)) {
        // Get that register.
        parse_register(opnd);

        if (token == TOK_RBRACKET) {

            // Simple indirect access with skipped constant.
            opnd->const_size = CS_SKIP;

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

            opnd->const_size = fit_size(offset);
            opnd->constant = offset;

            // Advance token to ']'.
            token = next_tok();
        } else {
            ERR_QUIT("Expected '+', '-', or ']'.");
        }

    } else if (token == TOK_NUM) {

        // Set data as offset indirect access.
        opnd->constant = lexint;
        opnd->const_size = fit_size(lexint);

        token = next_tok();
        // Next token should be a plus sign.
        if (token != TOK_PLUS)
            ERR_QUIT("Expected '+'.");

        // Next token should be a register.
        parse_register(opnd);
    }

    // Next token must be a closing bracket.
    if (token != TOK_RBRACKET) ERR_QUIT("Expected ']'.");

    // Overwrite any changes to type.
    opnd->type = OT_IND;

    // Advance to token after ']'.
    token = next_tok();
}

/*
 * Parses an assembler directive.
 *     e.g. `ds "hello\0"`
 *          `db 255, 42`
 * Pre-conditions: current token is one of TOK_DATA_SEG, ...
 * Post-conditions: current token is TOK_NL.
 */
static inline void parse_directive(void) {
    switch (token) {
        case TOK_DATA_STR:
            parse_data_str();
            break;
        case TOK_DATA_BYTE:
            parse_data_byte();
            break;
        case TOK_DATA_HALF:
            parse_data_half();
            break;
        case TOK_DATA_WORD:
            parse_data_word();
            break;
        default:
            ERR_QUIT("Bad stuff happened");
    }
}

/*
 * Parse `ds` directive.
 * Pre-conditions: current token is on `ds`.
 * Post-conditions: current token is TOK_NL.
 */
static void parse_data_str(void) {
    token = next_tok(); // Eat `ds`

    // String should follow.
    if (token != TOK_STR_LIT)
        ERR_QUIT("Expected string literal.");

    // String is in lexstr, with length in lexint.
    // Copy over to out_buffer and increment bytes.
    memcpy(out_buffer + loc_ctr, lexstr, lexint);
    loc_ctr += lexint;

    token = next_tok(); // Eat string literal.
}

/*
 * Parse `db` directive.
 * Pre-conditions: current token is on `db`.
 * Post-conditions: current token is TOK_NL.
 */
static void parse_data_byte(void) {
    // List of comma-delimited bytes follow `db`.
    while ((token = next_tok()) != TOK_NL) {
        if (token != TOK_NUM && token != TOK_CHR_LIT)
            ERR_QUIT("Expected numeric or character literal.");

        DEBUG(" { Reading byte %ld", lexint);

        // lexint contains numeric value of byte.
        if (lexint < SBYTE_MIN || UBYTE_MAX < lexint)
            ERR_QUIT("Number too large to fit in 8 bits.");

        out_buffer[loc_ctr++] = lexint; // Set into buffer

        // Advance past byte.
        token = next_tok();

        // TOK_NL signals end of list.
        if (token == TOK_NL) return;

        // Comma should follow if no newline.
        if (token != TOK_COMMA)
            ERR_QUIT("Expected `,` separator.");
    }
}

/*
 * Helper function for parse_data_half and parse_data_word.
 */
static void parse_data_num_list(int32_t low, uint32_t hi, int width) {
    // List of comma-delimited bytes follow `d_`.
    while ((token = next_tok()) != TOK_NL) {
        if (token != TOK_NUM)
            ERR_QUIT("Expected numeric literal.");

        // lexint contains numeric value of byte.
        if (lexint < low || hi < lexint)
            ERR_QUIT("Number too large to fit.");

        // Copy over to out_buffer and increment bytes.
        memcpy(out_buffer + loc_ctr, &lexint, width);
        loc_ctr += width;

        // Advance past byte.
        token = next_tok();

        // TOK_NL signals end of list.
        if (token == TOK_NL) return;

        // Comma should follow if no newline.
        if (token != TOK_COMMA)
            ERR_QUIT("Expected `,` separator.");
    }
}

/*
 * Parse `dh` directive.
 * Pre-conditions: current token is on `dh`.
 * Post-conditions: current token is TOK_NL.
 */
static void parse_data_half(void) {
    parse_data_num_list(SHALF_MIN, UHALF_MAX, HALF_WIDTH);
}

/*
 * Parse `dw` directive.
 * Pre-conditions: current token is on `dw`.
 * Post-conditions: current token is TOK_NL.
 */
static void parse_data_word(void) {
    parse_data_num_list(SWORD_MIN, UWORD_MAX, WORD_WIDTH);
}
