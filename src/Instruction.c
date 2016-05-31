
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "debug.h"
#include "Instruction.h"
#include "InstructionList.h"

/* list of instructions */
char * instrBuffer;
long instrPtr;
long instrCap;

int getInstrInfo(const char * name, InstrRecord * outRecord) {
    const InstrRecord * record = instrLookup;
    char upper[BUFSIZ];
    int i = 0;

    /* convert to uppercase for comparison */
    strcpy(upper, name);
    while (upper[i] != '\0') {
        upper[i] = toupper(upper[i]);
        i++;
    }

    /* loop through table, comparing names [yes this is O(n)...] */
    while (record->name != NULL) {

        if (0 == strcmp(upper, record->name)) {
            if (outRecord != NULL) *outRecord = *record;
            return 1; /* success */
        }

        record++; /* move to next record */
    }

    /* if not found, zero */
    return 0;
}

int isInstruction(const char * name) {
    return getInstrInfo(name, NULL);
}

int instructionSizeAgreement(Instruction * instr) {
    Operand * op1 = &instr->op1;
    Operand * op2 = &instr->op2;
    int result;

    /* don't need to do size agreement for no-op, one-op */
    if (instr->type == IT_N ||
        instr->type == IT_P ||
        instr->type == IT_U) {
        return 1;
    }

    /* "widen/narrow" operands if needed */
    if (op1->size != op2->size) {
        if (op1->type == OT_CONST) op1->size = OPSZ_LONG;
        else if (op2->type == OT_CONST) op2->size = OPSZ_LONG;

        if (instr->size != 0) {
            if (op1->type == OT_REG_ACCESS || op1->type == OT_REG_OFFSET)
                op1->size = instr->size;
            if (op2->type == OT_REG_ACCESS || op2->type == OT_REG_OFFSET)
                op2->size = instr->size;
        }
    }

    /* test that both sizes are the same */
    result = (op1->size == op2->size);

    /* if size is forced, check that the sizes agree with the forced size */
    if (instr->size != 0)
        result = (result && instr->size == op1->size);

    return result;
}

int instructionTypeAgreement(Instruction * instr) {
    Operand * op1 = &instr->op1;
    Operand * op2 = &instr->op2;

    /* check that the operands' sizes and types agree
     * with the instruction's prototype */
    switch (instr->type) {

        case IT_N: /* no operands */
            return op1->size == 0 && op2->size == 0;

        case IT_A:
            return op1->size != 0
                && op2->size != 0
                && op2->type != OT_CONST;

        case IT_B:
            return op1->size != 0
                && op2->size != 0
                && op1->type != OT_CONST;

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

        default:
            return 0;
    }
}

inline int hasCustomOffset(Operand * op) {
    OperandSize size = op->size;
    int offset = op->offset;

    if (op->type != OT_REG_OFFSET) return 0;

    if (offset < 0)
        offset = -offset;

    if (size == OPSZ_SHORT) {
        return offset != 0 &&
               offset != 1 &&
               offset != 2 &&
               offset != 3;
    } else {
        return offset != 0 &&
               offset != 4 &&
               offset != 8 &&
               offset != 12;
    }
}

/* returns the converted 3-bit special offset */
static char bitOffset(int offset) {
    switch (offset) {
        case 0: return R_OFF_0;

        case 1:
        case 4: return R_OFF_1_4;

        case 2:
        case 8: return R_OFF_2_8;

        case 3:
        case 12: return R_OFF_3_12;

        case -3:
        case -12: return R_NOFF_3_12;

        case -2:
        case -8: return R_NOFF_2_8;

        case -1:
        case -4: return R_NOFF_1_4;

        default: return R_OFF_CUSTOM;
    }
}

int saveInstruction(Instruction * instr) {
    int instruction = instr->opcode;
    int op1_const = 0;
    int op2_const = 0;

    char * temp;     /* for return from realloc */

    /* lay in size */
    if (instr->size == OPSZ_SHORT)
        instruction |= SIZE_BIT;

    /* lay in op types */
    Operand * op1 = &instr->op1;
    Operand * op2 = &instr->op2;

    instruction |= (op1->type << TYPE1_OFFSET);
    instruction |= (op2->type << TYPE2_OFFSET);

    DEBUG("  Final: Writing instr %s:0x%x with\n" \
          "\t  op1 type %d, value %d, offset %d\n" \
          "\t  op2 type %d, value %d, offset %d",
           instr->name, instr->opcode,
           op1->type, op1->value, op1->offset,
           op2->type, op2->value, op2->offset);

    /* operand 1*/
    if (op1->type == OT_CONST) {

        op1_const = op1->value; /* if the operand is a constant */

    } else if (op1->type == OT_REG_OFFSET) {

        if (hasCustomOffset(op1)) {
            op1_const = op1->offset;
        } else {
            instruction |= op1->value << OP1_OFFSET;
            instruction |= bitOffset(op1->offset) << (OP1_OFFSET + 4);
        }

    } else {
        instruction |= (op1->value << OP1_OFFSET);
    }

    /* operand 2 */
    if (op2->type == OT_CONST) {

        op2_const = op2->value; /* if the operand is a constant */

    } else if (op2->type == OT_REG_OFFSET) {

        if (hasCustomOffset(op2)) {
            op2_const = op2->offset;
        } else {
            instruction |= op2->value << OP2_OFFSET;
            instruction |= bitOffset(op2->offset) << (OP2_OFFSET + 4);
        }

    } else {
        instruction |= (op2->value << OP2_OFFSET);
    }

    /* TODO: make conditional */
    /* allocate more space for list if needed */
    instrCap = instrPtr + 3 * sizeof(int); /* increase capacity */

    /* realloc */
    temp = (char *) realloc(instrBuffer, instrCap);
    if (temp == NULL) {
        fprintf(stderr, "realloc() error.\n");
        return EXIT_FAILURE;
    }
    instrBuffer = temp;

    /* add instruction to buffer */
    memcpy(instrBuffer + instrPtr, &instruction, sizeof(instruction));
    instrPtr += sizeof(instruction);

    /* include any custom offsets/constants in succeeding word */
    if (op1->type == OT_CONST || hasCustomOffset(op1)) {
        memcpy(instrBuffer + instrPtr, &op1_const, sizeof(op1_const));
        instrPtr += sizeof(op1_const);
    }
    if (op2->type == OT_CONST || hasCustomOffset(op2)) {
        memcpy(instrBuffer + instrPtr, &op2_const, sizeof(op2_const));
        instrPtr += sizeof(op2_const);
    }

    return EXIT_SUCCESS;
}

int writeInstructions(FILE * stream) {
    fwrite(instrBuffer, sizeof(char), instrPtr, stream);
    return EXIT_SUCCESS; /* TODO: may not be successful? */
}
