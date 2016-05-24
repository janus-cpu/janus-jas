
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "debug.h"
#include "Instruction.h"
#include "InstructionList.h"

/* list of instructions */
Instruction * instructions;
long numinstrs;

/* count of instruction address location */
/* -1 since we pre-increment upon encountering a new instruction */
long lcounter = -1;

Instruction * saveInstruction(const char * name) {
    Instruction save = {0}; /* zero-fill */
    Instruction * temp;     /* for return from realloc */
    InstrRecord instructionInfo;

    /* allocate more space for list */
    temp = (Instruction *) realloc(instructions, sizeof(Instruction) * (numinstrs + 1));
    if (temp == NULL) { fprintf(stderr, "realloc() error.\n"); exit(1); }
    instructions = temp;

    /* populate new instruction */
    getInstrInfo(name, &instructionInfo);
    save.name = instructionInfo.name;
    save.type = instructionInfo.type;
    save.opcode = instructionInfo.opcode;

    /* insert into list */
    instructions[numinstrs] = save;

    DEBUG("instructions[%ld] inserted %s:0x%x",
            numinstrs, name, save.opcode);

    return (instructions + numinstrs++);
}

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

int instructionSizeAgreement(Instruction * instr) {
    Operand * op1 = &instr->op1;
    Operand * op2 = &instr->op2;
    int result;

    if (op1->size != op2->size) {
        /* widen numbers if needed */
        if (op1->type == OT_CONST) op1->size = OPSZ_LONG;
        else if (op2->type == OT_CONST) op2->size = OPSZ_LONG;
    }

    result = (op1->size == op2->size);

    if (instr->size != 0)
        result = (result && instr->size == op1->size);

    return result;
}

int instructionTypeAgreement(Instruction * instr) {
    Operand * op1 = &instr->op1;
    Operand * op2 = &instr->op2;

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

#define PLACE_SIZE(sz) ((sz) == OPSZ_LONG ? 0 : 1)
#define PLACE_TYPE(t,n) ()
#define SIZE_BIT 0x00000200

int writeInstruction(Instruction * instr, FILE * stream) {
    int instruction = instr->opcode;
    int constant = 0;

    /* put size */
    if (instr->size == OPSZ_SHORT)
        instruction |= SIZE_BIT;

    /* put op types */
    Operand * op1 = &instr->op1;
    Operand * op2 = &instr->op2;

    instruction |= (op1->type) << 14;
    instruction |= (op2->type) << 16;

    /* op values */
    if (op1->type != OT_CONST)
        instruction |= (op1->value << 18);
    else
        constant = op1->value;

    if (op2->type != OT_CONST)
        instruction |= op2->value << 25;
    else
        constant = op2->value;

    fwrite(&instruction, sizeof(int), 1, stream);
    if (op1->type == OT_CONST || op2->type == OT_CONST)
        fwrite(&constant, sizeof(int), 1, stream);

    fprintf(stderr, "OPERATION:: %08x\nCONSTANT:: %08x\n", instruction, constant);
    return 0;
}

int isInstruction(const char * name) {
    return getInstrInfo(name, NULL);
}
