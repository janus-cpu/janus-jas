#include "parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

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
static inline bool parse_line(void);
static inline bool parse_label(void);
static inline bool parse_instruction(void);
static inline bool parse_directive(void);

static bool parse_length_modifier(struct Instruction * instr);
static bool parse_operands(struct Instruction * instr);
static bool parse_operand(struct Operand * opnd);
static bool parse_register(struct Operand * opnd);
static bool parse_register_indirect(struct Operand * opnd);
static bool parse_data_str(void);
static bool parse_data_byte(void);
static bool parse_data_half(void);
static bool parse_data_word(void);

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
static inline bool parse_line(void) {
    // Let by empty lines.
    if (token == TOK_NL) return true;

    if (token == TOK_LABEL) {

        EXPECT(parse_label());

    } else if (token == TOK_INSTR) {

        EXPECT(parse_instruction());

    } else if (is_directive(token)) {

        EXPECT(parse_directive());

    } else {
        ERR_FLUSH("Line must start with label, instruction, or data segment.");
    }

    return true;
}

/*
 * Parse a label.
 *     e.g. `_L0:`
 * Pre-conditions: current token is TOK_LABEL.
 * Post-conditions: current token is the one following the label and its colon.
 */
static inline bool parse_label(void) {
    save_label(lexstr);
    return true;
}

/*
 * Parse a whole instruction (opcode and operands).
 * Pre-conditions: current token is TOK_INSTR.
 * Post-conditions: current token is the one following the last operand of the
 *                  instruction.
 */
static inline bool parse_instruction(void) {
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
    // Assume size is long by default.
    newInstr.size = OS_LONG;
    if (token == TOK_DOT) {
        EXPECT(parse_length_modifier(&newInstr));
    }

    // Read the operands.
    EXPECT(parse_operands(&newInstr));

    // Check that the operand types agree with the instruction.
    if (!instr_type_agreement(&newInstr)) {
        jas_err_line("Instruction operands do not agree with its prototype.",
                instr_line, instr_lo_col, instr_curr_col);
    }

    // If instruction is synthetic, turn it into its aliased instruction.
    // XXX: What about synthetic instructions that expand into more than one?
    unalias_instruction(&newInstr);

    // FIXME: should these checks should be in the analyze() function?
    // Semantic checks
    DEBUG(" \\ Semantic: checking `%s` ty=%d sz=%d sz1=%d sz2=%d",
            newInstr.name,     newInstr.type, newInstr.size,
            newInstr.op1.size, newInstr.op2.size);

    if (!instr_size_agreement(&newInstr)) {
        jas_err_line("Instruction operands' sizes are not in agreement.",
                instr_line, instr_lo_col, instr_curr_col);
    }

    // XXX: is this a good place to write out the instruction?
    /* write the machine code for this instruction into the buffer */
    if (!j_err && save_instruction(&newInstr)) {
        jas_err_line("Could not write instruction!",
                instr_line, instr_lo_col, instr_curr_col);
        return false;
    }

    return true;
}

/*
 * Parse the length modifier for an instruction.
 *     e.g. `.l` in `mov.l`
 * Pre-conditions: current token is TOK_DOT.
 * Post-conditions: current token is the one following the length modifier.
 *                  `instr` will contain size information of the instruction.
 */
static bool parse_length_modifier(struct Instruction * instr) {
    token = next_tok(); // Advance to length modifier.

    // NOP-type can't receive any length modifier suffix.
    if (instr->type == IT_N || fixed_instruction(instr->opcode)) {
        ERR_FLUSH("Instruction cannot have a length modifer.");
    }

    if (token == TOK_ID && *lexstr == 's') {
        instr->size = OS_SHORT;
        instr->opcode++; // Increment to short opcode.
    } else if (token == TOK_ID && *lexstr == 'l') {
        instr->size = OS_LONG;
    } else {
        ERR_FLUSH("Invalid length modifier, expecting 's' or 'l'.");
    }

    DEBUG(" | Forced op size %d.", instr->size);

    token = next_tok(); // Advance to operands.
    return true;
}

/*
 * Parse multiple operands for an instruction.
 * Pre-conditions: current token is /possibly/ the start of an operand.
 * Post-conditions: current token is TOK_NL.`instr` will contain information
 *                  about the operands.
 */
static inline bool parse_operands(struct Instruction * instr) {
    // Because there are no-operand, one-operand, and two-operand instructions,
    // there is a possibility of TOK_NL appearing throughout parsing these.

    if (token == TOK_NL) return true; // No-operand instruction.

    // Check consistency with instruction type.
    if (!has_operands(instr->type))
        ERR_FLUSH("Instruction prototype doesn't have operands.");

    DEBUG(" - Reading first operand%s","");

    // First operand.
    EXPECT(parse_operand(&instr->op1));

    if (token == TOK_NL) return true; // One-operand instruction.

    // Check consistency with instruction type.
    if (!has_two_operands(instr->type))
        ERR_FLUSH("Instruction prototype doesn't have two operands.");

    // Expect a comma before a second operand.
    if (token != TOK_COMMA) ERR_FLUSH("Expected comma.");

    DEBUG(" - Reading second operand%s","");

    // Advance to next operand.
    token = next_tok();
    EXPECT(parse_operand(&instr->op2)); // Two-operand instruction.

    return true;
}

/*
 * Parse a single operand.
 * Pre-conditions: current token is /possibly/ the start of an operand.
 * Post-conditions: current token is one following the operand.
 *                  `opnd` will contain information about the operand.
 */
static bool parse_operand(struct Operand * opnd) {

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

        EXPECT(parse_register(opnd));

    } else if (token == TOK_LBRACKET) {

        EXPECT(parse_register_indirect(opnd));

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
            save_undef_label(lexstr, 2); // 2 offset = 1 opcode + 1 descriptor
        }

        // Advance token past the identifier.
        token = next_tok();

    } else {
        ERR_FLUSH("Unrecognizable operand.");
    }

    return true;
}

/*
 * Parses a register.
 *     e.g. `r0`, `r0a`, `rk0`
 * Pre-conditions: current token is one of TOK_GL_REG, TOK_GS_REG, TOK_E_REG,
 *                 or TOK_K_REG.
 * Post-conditions: current token is the one following the register token.
 *
 */
static bool parse_register(struct Operand * opnd) {
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

    return true;
}

/*
 * Accumulates 'scale' into either reg1_* or reg2_* based on register 'ID',
 * either matching or claiming an empty space if possible.
 */
static bool reg_accumulate(
    RegisterId id,        int scale,
    RegisterId * reg1_id, int * reg1_scale,
    RegisterId * reg2_id, int * reg2_scale
) {
    // First see if our id sits in reg1 or reg2.
    // If it doesn't, then check if reg1 or reg2 are empty.
    // If they are, fill it, otherwise, be sad.
    if (*reg1_id == id) {
        *reg1_scale += scale;
    } else if (*reg2_id == id) {
        *reg2_scale += scale;
    } else if (*reg1_id == (RegisterId) -1) {
        *reg1_id = id;
        *reg1_scale = scale;
    } else if (*reg2_id == (RegisterId) -1) {
        *reg2_id = id;
        *reg2_scale = scale;
    } else {
        ERR_FLUSH("Cannot process indirect with 3 operands.");
    }

    return true;
}

/*
 * Parses an indirect memory access, with an offset or not.
 *     e.g. `[r0]` or `[r0 + 4]` or `[-4 + r0]`
 * Pre-conditions: current token is TOK_LBRACKET.
 * Post-conditions: current token is after the TOK_RBRACKET of the indirect.
 *                  `opnd` will contain information about the operand.
 */
static bool parse_register_indirect(struct Operand * opnd) {
    bool first = true;
    int constant = 0;
    int reg1_scale = 0, reg2_scale = 0;
    RegisterId reg1_id = -1, reg2_id = -1;

    token = next_tok();

    while (true) {
        if (!first) {
            if (token == TOK_RBRACKET) {
                token = next_tok();
                break;
            } else if (token == TOK_PLUS) {
                token = next_tok();
            } else {
                ERR_FLUSH("This .... uhhh.. should be unreachable... (:");
            }
        } else
            first = false;

        if (token == TOK_NUM) {
            int scale = lexint;
            token = next_tok();

            if (token == TOK_STAR) {
              RegisterId id;

              if (next_tok() != TOK_NUM) {
                  ERR_FLUSH("Expected number following offset multiplication.");
              }

              id = register_id(lexstr);
              reg_accumulate(id, scale, &reg1_id, &reg1_scale, &reg2_id, &reg2_scale);
              token = next_tok();
            } else if (token == TOK_RBRACKET || token == TOK_PLUS) {
                constant += scale;
            } else {
                ERR_FLUSH("Expected `+`, `*`, or `]`.");
            }
        } else if (token == TOK_GL_REG) {
            RegisterId id = register_id(lexstr);
            DEBUG("Read reg %d", id);
            token = next_tok();

            if (token == TOK_STAR) {
                int scale;

                if (next_tok() != TOK_NUM) {
                    ERR_FLUSH("Expected number following register multiplication.");
                }

                scale = lexint;
                reg_accumulate(id, scale, &reg1_id, &reg1_scale, &reg2_id, &reg2_scale);
                token = next_tok();
            } else if (token == TOK_RBRACKET || token == TOK_PLUS) {
                reg_accumulate(id, 1, &reg1_id, &reg1_scale, &reg2_id, &reg2_scale);
            } else {
                ERR_FLUSH("Expected `+`, `*`, or `]`.");
            }
        } else {
            ERR_FLUSH("Expected integer or long register.");
        }
    }

    if (reg2_id == (RegisterId) -1) {
        if (reg1_id == (RegisterId) -1)
            ERR_FLUSH("Need at least 1 register in an indirect access.");

        if (reg1_scale == 1) {
            opnd->type = OT_IND;
            opnd->reg = reg1_id;
        } else if (reg1_scale == 2 || reg1_scale == 3 ||
                   reg1_scale == 5 || reg1_scale == 9) {
            opnd->type = OT_SC_IND;
            opnd->reg = opnd->index_reg = reg1_id;
            opnd->scale = reg1_scale - 1;
        } else {
            ERR_FLUSH("TODO: pls better error msg.");
        }
    } else {
        if (reg1_scale == 1) {
            if (reg2_scale != 1 && reg2_scale != 2 &&
                reg2_scale != 4 && reg2_scale != 8)
                ERR_FLUSH("Offset register needs to be power of 2.");
            opnd->type = OT_SC_IND;
            opnd->reg = reg1_id;
            opnd->index_reg = reg2_id;
            opnd->scale = reg2_scale;
        } else if (reg2_scale == 1) {
            if (reg1_scale != 1 && reg1_scale != 2 &&
                reg1_scale != 4 && reg1_scale != 8)
                ERR_FLUSH("Offset register needs to be power of 2.");
            opnd->type = OT_SC_IND;
            opnd->reg = reg2_id;
            opnd->index_reg = reg1_id;
            opnd->scale = reg1_scale;
        } else {
          ERR_FLUSH("TODO: no base :(");
        }
    }

    opnd->constant = constant;
    opnd->const_size = fit_size(constant);
    opnd->size = OS_SHORT;

    return true;
}

/*
 * Parses an assembler directive.
 *     e.g. `ds "hello\0"`
 *          `db 255, 42`
 * Pre-conditions: current token is one of TOK_DATA_SEG, ...
 * Post-conditions: current token is TOK_NL.
 */
static inline bool parse_directive(void) {
    switch (token) {
        case TOK_DATA_STR:
            EXPECT(parse_data_str());
            break;
        case TOK_DATA_BYTE:
            EXPECT(parse_data_byte());
            break;
        case TOK_DATA_HALF:
            EXPECT(parse_data_half());
            break;
        case TOK_DATA_WORD:
            EXPECT(parse_data_word());
            break;
        default:
            ERR_FLUSH("Bad stuff happened...");
    }

    return true;
}

/*
 * Parse `ds` directive.
 * Pre-conditions: current token is on `ds`.
 * Post-conditions: current token is TOK_NL.
 */
static bool parse_data_str(void) {
    token = next_tok(); // Eat `ds`

    // String should follow.
    if (token != TOK_STR_LIT)
        ERR_FLUSH("Expected string literal.");

    // String is in lexstr, with length in lexint.
    // Copy over to out_buffer and increment bytes.
    memcpy(out_buffer + loc_ctr, lexstr, lexint);
    loc_ctr += lexint;

    token = next_tok(); // Eat string literal.

    return true;
}

/*
 * Parse `db` directive.
 * Pre-conditions: current token is on `db`.
 * Post-conditions: current token is TOK_NL.
 */
static bool parse_data_byte(void) {
    // List of comma-delimited bytes follow `db`.
    while ((token = next_tok()) != TOK_NL) {
        if (token != TOK_NUM && token != TOK_CHR_LIT)
            ERR_FLUSH("Expected numeric or character literal.");

        DEBUG(" { Reading byte %ld", lexint);

        // lexint contains numeric value of byte.
        if (UBYTE_MAX < (uint32_t) lexint)
            ERR_FLUSH("Number too large to fit in 8 bits.");

        out_buffer[loc_ctr++] = lexint; // Set into buffer

        // Advance past byte.
        token = next_tok();

        // TOK_NL signals end of list.
        if (token == TOK_NL) return true;

        // Comma should follow if no newline.
        if (token != TOK_COMMA)
            ERR_FLUSH("Expected `,` separator.");
    }

    return true;
}

/*
 * Helper function for parse_data_half and parse_data_word.
 */
static bool parse_data_num_list(uint32_t hi, int width) {
    // List of comma-delimited bytes follow `d_`.
    while ((token = next_tok()) != TOK_NL) {
        if (token != TOK_NUM)
            ERR_FLUSH("Expected numeric literal.");

        // lexint contains numeric value of byte.
        if (hi < (uint32_t) lexint)
            ERR_FLUSH("Number too large to fit.");

        // Copy over to out_buffer and increment bytes.
        memcpy(out_buffer + loc_ctr, &lexint, width);
        loc_ctr += width;

        // Advance past byte.
        token = next_tok();

        // TOK_NL signals end of list.
        if (token == TOK_NL) return true;

        // Comma should follow if no newline.
        if (token != TOK_COMMA)
            ERR_FLUSH("Expected `,` separator.");
    }

    return true;
}

/*
 * Parse `dh` directive.
 * Pre-conditions: current token is on `dh`.
 * Post-conditions: current token is TOK_NL.
 */
static bool parse_data_half(void) {
    return parse_data_num_list(UHALF_MAX, HALF_WIDTH);
}

/*
 * Parse `dw` directive. Can contain numeric literals or labels.
 * Pre-conditions: current token is on `dw`.
 * Post-conditions: current token is TOK_NL.
 */
static bool parse_data_word(void) {
    // List of comma-delimited bytes follow `dw`.
    while ((token = next_tok()) != TOK_NL) {
        if (token != TOK_NUM && token != TOK_ID)
            ERR_FLUSH("Expected numeric literal or identifier.");

        // Try to get an address for a label, or just save it at 0 offset.
        if (token == TOK_ID) {
            lexint = label_address(lexstr);
            if (lexint == -1) save_undef_label(lexstr, 0);
        }

        // lexint contains numeric value of word.
        if (UWORD_MAX < (uint32_t) lexint)
            ERR_FLUSH("Number too large to fit.");

        // Copy over to out_buffer and increment bytes.
        memcpy(out_buffer + loc_ctr, &lexint, WORD_WIDTH);
        loc_ctr += WORD_WIDTH;

        // Advance past byte.
        token = next_tok();

        // TOK_NL signals end of list.
        if (token == TOK_NL) return true;

        // Comma should follow if no newline.
        if (token != TOK_COMMA)
            ERR_FLUSH("Expected `,` separator.");
    }

    return true;
}
