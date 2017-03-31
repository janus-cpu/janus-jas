#include <stdio.h>
#include <stdlib.h>
#include <strings.h> // For strcasecmp()
#include <string.h>
#include <ctype.h>

#include "debug.h"
#include "util.h"
#include "instruction.h"
#include "instruction_list.h"

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

/*
 * TODO
 */
int instructionSizeAgreement(struct Instruction * instr) {
    struct Operand * op1 = &instr->op1;
    struct Operand * op2 = &instr->op2;
    int result;

    /* don't need to do size agreement for no-op */
    if (instr->type == IT_N) return 1;

    // Force operand size for a register indirect.
    if (instr->type == IT_P ||
        instr->type == IT_U ||
        instr->type == IT_T) {
        // If instruction size is undetermined, make it match the operand.
        if (instr->size == 0) {
            instr->size = op1->size;
            return 1;
        }

        // Constants and registers must agree with instruction size.
        if ((op1->type == OT_CONST || op1->type == OT_REG)
            && op1->size != instr->size)
            return 0;

        // Indirects are more flexible with size.
        if (op1->type == OT_IND || op1->type == OT_SC_IND) {
            op1->size = instr->size;
        }

        return 1;
    }

    /* "widen/narrow" operands if needed */
    if (instr->size != 0) {
        DEBUG("  | Matching indirect op to size %d", instr->size);
        // If operands are indirects, it's easy to make them conform to
        // the instruction's size.
        if (op1->type == OT_IND || op1->type == OT_SC_IND)
            op1->size = instr->size;
        if (op2->type == OT_IND || op2->type == OT_SC_IND)
            op2->size = instr->size;
    }

    // Widen constants to match the wider operand.
    if (op1->type == OT_CONST && op1->size < op2->size)
        op1->size = op2->size;
    else if (op2->type == OT_CONST && op2->size < op1->size)
        op2->size = op1->size;

    // Check if manipulations above matched the two sizes.
    result = (op1->size == op2->size);

    // If size is forced, check that the sizes agree with the forced size.
    if (instr->size != 0)
        result = (result && instr->size == op1->size);
    else
        instr->size = op1->size; // Indeterminate instr size matches operands.

    return result;
}

/*
 * TODO
 */
int instructionTypeAgreement(struct Instruction * instr) {
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
