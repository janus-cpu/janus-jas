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

/* Special offsets for indirect access */
#define R_OFF_0      0
#define R_OFF_1_4    1
#define R_OFF_2_8    2
#define R_OFF_3_12   3
#define R_NOFF_3_12  4
#define R_NOFF_2_8   5
#define R_NOFF_1_4   6
#define R_OFF_CUSTOM 7

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
    Operand op1;
    Operand op2;
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
extern Instruction * instructions; /* FIXME: get rid of eventually lmeaux */
extern char * instrBuffer; /* use this to write directly to file */
extern long instrPtr; /* points to next available (byte) space in the buffer */
extern long instrCap;

/* location counter */
extern long lcounter;

/** function prototypes **/
/* looking up instructions */
int getInstrInfo(const char * name, InstrRecord * out);
int isInstruction(const char * name);
int hasCustomOffset(Operand * op);

/* saving and writing instructions */
int saveInstruction(Instruction * instr);
int instructionSizeAgreement(Instruction * instr);
int instructionTypeAgreement(Instruction * instr);
int writeInstructions(FILE * stream);

#endif
