#include <stdio.h>
#include <stdlib.h>
#include <strings.h> // For strcasecmp()
#include <string.h>
#include <ctype.h>

#include "debug.h"
#include "util.h"
#include "instruction.h"
#include "instruction_list.h"
#include "jas_limits.h"

/*
 * Get the corresponding InstrRecord for the instruction name.
 * Returns 1 if successful, 0 if no record can be found.
 */
int instr_info(const char * name, struct InstrRecord * info) {
    const struct InstrRecord * record = instr_lookup;

    // Loop through table, comparing names [yes this is O(n)...].
    while (record->name != NULL) {
        // Set data into out parameter if names match.
        if (0 == strcasecmp(name, record->name)) {
            if (info != NULL) *info = *record;
            return 1;
        }

        record++;
    }

    // Record not found.
    return 0;
}

/*
 * Returns 1 if name is an instruction, 0 if not.
 */
int is_instruction(const char * name) {
    return instr_info(name, NULL);
}

/*
 * Transform synthetic instruction into the actual instruction.
 */
void unalias_instruction(struct Instruction * instr) {
    const char * name = instr->name;
    struct InstrRecord info = {0};

    // NOP => XCHG r0, r0
    if (!strcmp(name, "NOP")) {
        instr_info("XCHG", &info);

        // Fill with aliased instr info.
        instr->name = info.name;
        instr->type = info.type;
        instr->opcode = info.opcode;

        // Set operands to r0, r0
        struct Operand r0 = {
            .type = OT_REG,
            .size = OS_LONG,
            .reg = 0
        };

        instr->op1 = r0;
        instr->op2 = r0;
    }

    // INC op => ADD 1, op
    if (!strcmp(name, "INC")) {
        instr_info("ADD", &info);

        // Fill with aliased instr info.
        instr->name = info.name;
        instr->type = info.type;
        instr->opcode = info.opcode;

        // Set operands to 1, op
        struct Operand const1 = {
            .type = OT_CONST,
            .size = OS_SHORT,
            .const_size = CS_BYTE,
            .constant = 1,
        };

        instr->op2 = instr->op1;
        instr->op1 = const1;
    }

    // DEC op => SUB 1, op
    if (!strcmp(name, "DEC")) {
        instr_info("SUB", &info);

        // Fill with aliased instr info.
        instr->name = info.name;
        instr->type = info.type;
        instr->opcode = info.opcode;

        // Set operands to 1, op
        struct Operand const1 = {
            .type = OT_CONST,
            .size = OS_SHORT,
            .const_size = CS_BYTE,
            .constant = 1,
        };

        instr->op2 = instr->op1;
        instr->op1 = const1;
    }

    // NEG op => SUB 0, op
    if (!strcmp(name, "NEG")) {
        instr_info("SUB", &info);

        // Fill with aliased instr info.
        instr->name = info.name;
        instr->type = info.type;
        instr->opcode = info.opcode;

        // Set operands to 0, op
        struct Operand const0 = {
            .type = OT_CONST,
            .size = OS_SHORT,
            .const_size = CS_BYTE,
            .constant = 0,
        };

        instr->op2 = instr->op1;
        instr->op1 = const0;
    }

    // CLR op => XOR op, op
    if (!strcmp(name, "CLR")) {
        instr_info("XOR", &info);

        // Fill with aliased instr info.
        instr->name = info.name;
        instr->type = info.type;
        instr->opcode = info.opcode;

        // Operands are both op1.
        instr->op2 = instr->op1;
    }
}

/*
 * Check if instruction opcode is only long type. i.e. within ranges
 *  - [0x80, 0x8A]: jmp and call Instructions
 *  - [0x70, 0x7D]: special register load/read instructions
 */
inline int is_long_instruction(unsigned char opcode) {
    return (0x80 <= opcode && opcode <= 0x8A) ||
           (0x70 <= opcode && opcode <= 0x7D);
}

inline int togglable_instruction(unsigned char opcode) {
    return opcode < 0x70 || 0x8F < opcode; // Not not togglable.
}

// Sets the constant size for an OT_CONST operand.
void set_op_const_size(struct Operand * op, enum ConstantSize const_size) {
    op->const_size = const_size;
    op->size = const_size > 1 ? OS_LONG : OS_SHORT;
}

/*
 * Try to get the operand to agree with the instruction size somehow.
 * Returns 1 if agreement successful, 0 if not.
 */
static int op_size_agreement(struct Instruction * instr, struct Operand * op) {
    // Get signed constant.
    int sign_const = op->constant;

    // Skip empty operands.
    if (op->size == 0) return 1;

    // If constant operand(s) and signed, we can narrow to short instr size.
    if (op->type == OT_CONST) {
        DEBUG("  | Constant operand sz=%d", op->size);

        // First, fit positive constants into as small of a space as possible.
        if (sign_const >= 0)
            set_op_const_size(op, fit_size(op->constant));

        // Truncate/fit constant for short instr, widen for long.
        if (instr->size == OS_SHORT && sign_const < 0 && sign_const >= SBYTE_MIN)
            set_op_const_size(op, CS_BYTE);
        else if (instr->size == OS_LONG)
            set_op_const_size(op, fit_size_hint(op->constant, instr->size));

    // If operands are indirects, make them conform to the instruction's size.
    // Negative indices/offsets have a default 4 byte size, so no check needed.
    } else if (op->type == OT_IND || op->type == OT_SC_IND) {
        op->size = instr->size;
    }

    return op->size == instr->size;
}

/*
 * Check for size agreement with instruction and operands, manipulating operands
 * size to try to fit, if possible.
 */
int instr_size_agreement(struct Instruction * instr) {
    struct Operand * op1 = &instr->op1;
    struct Operand * op2 = &instr->op2;
    int result; // Agreement or not.

    // NOP type doesn't need agreement.
    if (instr->type == IT_N) return 1;

    result = op_size_agreement(instr, op1);
    result = op_size_agreement(instr, op2) && result;

    // Widen constants to match the wider operand.
    if (op1->type == OT_CONST && op1->size < op2->size)
        op1->size = op2->size;
    else if (op2->type == OT_CONST && op2->size < op1->size)
        op2->size = op1->size;

    // Check if manipulations above matched the two sizes.
    result = result && (op1->size == op2->size);

    return result;
}

/*
 * Check that the instruction's operand types agree with the instruction's
 * prototype.
 */
int instr_type_agreement(struct Instruction * instr) {
    struct Operand * op1 = &instr->op1;
    struct Operand * op2 = &instr->op2;

    // Check that the operands' sizes and types agree
    // with the instruction's prototype.
    switch (instr->type) {

        case IT_N: /* no operands */
            return op1->size == 0 && op2->size == 0;

        case IT_A:
            return op1->size != 0
                && op2->size != 0
                && op2->type != OT_CONST;

        case IT_X:
            return op1->size != 0
                && op2->size != 0
                && op1->type != OT_CONST
                && op2->type != OT_CONST;

        case IT_I:
            return op1->size != 0
                && op2->size != 0
                && op1->type == OT_CONST
                && op2->type != OT_CONST;

        case IT_P:
            return op1->size != 0
                && op2->size == 0
                && op1->type != OT_CONST;

        case IT_U:
            return op1->size != 0
                && op2->size == 0;

        case IT_T:
            return op1->size != 0
                && op2->size == 0
                && op1->type == OT_CONST;

        default:
            return 0;
    }
}

/*
 * Check if instruction type has any operands.
 */
inline int has_operands(enum InstructionType ty) {
    return ty != IT_N;
}

/*
 * Check if instruction type has exactly two operands.
 */
inline int has_two_operands(enum InstructionType ty) {
    return has_operands(ty) && (ty == IT_A || ty == IT_X || ty == IT_I);
}
