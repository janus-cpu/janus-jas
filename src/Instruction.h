#ifndef INSTRUCTION_H
#define INSTRUCTION_H
/*
 * Header for Instruction infrastructureses
 * ----------------------------------------
 *
 * Line of assembly:
 * [LABEL] MNEMONIC [OPERAND, ...] [COMMENT]
 *
 * Operand Prototypes:
 * N - NOP  (no operands)
 * A - ADD  any,     reg_ind
 * B - CMP  reg_ind, any
 * X - XCHG reg_ind, reg_ind
 * I - IN   const,   reg_ind
 * P - POP  reg_ind
 *
 */

/* these only need to be a byte long */
typedef char OperandType;
typedef char InstructionType;
typedef char OperandSize;

///** opcodes for instructions! TODO: might become unnecessary **/
//enum OpCode {
//    OP_NOP   = 0x00,    /* no-op */
//
//    OP_ADD   = 0x01,    /* simple arithmetic */
//    OP_ADC   = 0x02,
//    OP_SUB   = 0x03,
//    OP_SBB   = 0x04,
//
//    OP_CMP  = 0x05,     /* comparison and testing:             */
//    OP_TEST = 0x07,
//
//    OP_DEC   = 0x09,    /* decrement/increment */
//    OP_INC   = 0x0A,
//
//    OP_UDIV  = 0x0B,    /* unsigned and signed multiplication/division */
//    OP_UMUL  = 0x0C,
//    OP_SDIV  = 0x0D,
//    OP_SMUL  = 0x0E,
//
//    OP_NEG   = 0x0F,    /* numerical negation */
//
//    OP_NOT   = 0x10,    /* bitwise operations */
//    OP_AND   = 0x11,
//    OP_OR    = 0x12,
//    OP_XOR   = 0x13,
//
//    OP_JMP   = 0x30,    /* control flow */
//
//
//    OP_MOV   = 0x50     /* miscellaneous */
//
//};

/** Operand-relevant declarations **/
/* Enumeration of the 4 operand types */
enum OperandType {
    OT_REG,         /* register                                   */
    OT_REG_ACCESS,  /* [register]                                 */
    OT_REG_OFFSET,  /* [register + offset] OR [offset + register] */
    OT_CONST        /* constant                                   */
};

/* Indeterminant(instruction-only), long, or short */
enum OperandSize {
    OPSZ_INDET,     /* only for Instructions that don't force size */
    OPSZ_LONG,      /* 32-bit long register argument               */
    OPSZ_SHORT      /* 8-bit  short register argument              */
};

/* Operand type declaration */
typedef struct {
    OperandType type;
    OperandSize size;
    int value;  /* the value of the register                           */
    int offset; /* how much of an offset, use dependent on OperandType */
} Operand;

/** Instruction-relevant declarations **/
/* Enumeration of instruction prototype variants */
enum InstructionType {
    IT_N, /* (no operands)    */
    IT_A, /* any,     reg_ind */
    IT_B, /* reg_ind, any     */
    IT_X, /* reg_ind, reg_ind */
    IT_I, /* const,   reg_ind */
    IT_P, /* reg_ind          */
    IT_U  /* any              */
};

/* Instruction type declaration */
typedef struct {
    const char * name;
    InstructionType type;
    OperandSize size;     /* ATTN!: only non-zero if size forced with .s/.l  */
    short opcode;         /* short since an opcode is <= 9 bits in length    */
    Operand op1;          /* XXX: is it limiting to assume only 2 operands?  */
    Operand op2;          /*      will we ever have more than 2???           */
} Instruction;

/* this is just a record for the mnemonic lookup table */
typedef struct {
    const char * name;     /* name of the mnemonic */
    short opcode;
    InstructionType type;
} InstrRecord;

/** Extern declarations **/
/* mnemonics table */
extern const InstrRecord instrLookup[];

/* instructions list */
extern Instruction * instructions;
extern long numinstrs;

/* location counter */
extern long lcounter;

/** function prototypes **/
/* looking up instructions */
int getInstrInfo(const char * name, InstrRecord * out);
int isInstruction(const char * name);

/* saving and writing instructions */
Instruction * saveInstruction(const char * name);
int instructionSizeAgreement(Instruction * instr);
int instructionTypeAgreement(Instruction * instr);
int writeInstruction(Instruction * instr, FILE * stream);

#endif
