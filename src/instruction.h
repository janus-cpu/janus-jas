#ifndef INSTRUCTION_H
#define INSTRUCTION_H

/*
 * Header for Instruction infrastructure
 * ----------------------------------------
 *
 * Line of assembly:
 * [LABEL] MNEMONIC [OPERAND, ...] [COMMENT]
 *
 * Operand Prototypes:
 * N - NOP  (no operands)
 * A - ADD  any,     reg_ind
 * X - XCHG reg_ind, reg_ind
 * I - IN   const,   reg_ind
 * P - POP  reg_ind
 * U - PUSH any
 * T - INT  const
 *
 */

/** Operand-relevant declarations **/
/* Enumeration of the 4 operand types */
enum OperandType {
    OT_CONST  = 0, // constant
    OT_IND    = 1, // [register + offset]
    OT_REG    = 2, // register
    OT_SC_IND = 3  // [register + scale*index + offset]
};

/* Indeterminant(instruction-only), long, or short */
enum OperandSize {
    OS_INDET = 0, // only for Instructions that don't force size
    OS_LONG  = 4, // 32-bit long register argument
    OS_SHORT = 1  // 8-bit  short register argument
};

/*
 * Size of constant in operand.
 */
enum ConstantSize {
    CS_SKIP = 0, // Skipped constant
    CS_BYTE = 1, // 1 byte constant
    CS_HALF = 2, // 2 byte constant
    CS_WORD = 3  // 4 byte constant
};

/* Operand type declaration */
struct Operand {
    enum OperandType type;
    enum OperandSize size;
    enum ConstantSize const_size;

    int constant;            // Constant, may be skipped
    unsigned char reg;       // Base register for a register operand
    unsigned char index_reg; // Offset register
    unsigned char scale;     // Scale multiplied against index
};

/** Instruction-relevant declarations **/
/* Enumeration of instruction prototype variants */
enum InstructionType {
    IT_N, /* (no operands)    */
    IT_A, /* any,     reg_ind */
    IT_X, /* reg_ind, reg_ind */
    IT_I, /* const,   reg_ind */
    IT_P, /* reg_ind          */
    IT_U, /* any              */
    IT_T  /* const            */
};

/* Instruction type declaration */
struct Instruction {
    const char * name;
    enum InstructionType type;
    unsigned char opcode;

    enum OperandSize size;
    struct Operand op1;
    struct Operand op2;
};

/* this is just a record for the mnemonic lookup table */
struct InstrRecord {
    const char * name;
    unsigned char opcode;
    enum InstructionType type;
};

#define MAX_INSTR_SIZE 13 // 1 + (2 + 4) + (2 + 4)
#define OPCODE_INT 0x8E   // For easy comparison to INT opcode

/** Extern declarations **/

/** function prototypes **/
/* looking up instructions */
int instr_info(const char * name, struct InstrRecord * out);

int is_instruction(const char * name);
int is_long_instruction(unsigned char opcode);
int togglable_instruction(unsigned char opcode);
int has_operands(enum InstructionType ty);
int has_two_operands(enum InstructionType ty);

/* saving and writing instructions */
void unalias_instruction(struct Instruction * instr);
int instructionSizeAgreement(struct Instruction * instr);
int instructionTypeAgreement(struct Instruction * instr);

#endif
