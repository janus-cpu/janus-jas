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
 * T - INT  const
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
    OPSZ_INDET = 0,     /* only for Instructions that don't force size */
    OPSZ_LONG  = 4,     /* 32-bit long register argument               */
    OPSZ_SHORT = 1      /* 8-bit  short register argument              */
};

/* Operand type declaration */
struct Operand {
    enum OperandType type;
    enum OperandSize size;
    int value;  /* the value of the register                           */
    int offset; /* how much of an offset, use dependent on OperandType */
};

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
    IT_U, /* any              */
    IT_T  /* const            */
};

/* Instruction type declaration */
struct Instruction {
    const char * name;
    enum InstructionType type;
    OperandSize size;     /* ATTN!: only non-zero if size forced with .s/.l  */
    short opcode;         /* short since an opcode is <= 9 bits in length    */
    struct Operand op1;
    struct Operand op2;
};

/* this is just a record for the mnemonic lookup table */
struct InstrRecord{
    const char * name;     /* name of the mnemonic */
    short opcode;
    enum InstructionType type;
};

/* Bit offsets for saving the instruction */
#define SIZE_BIT     0x00000200
#define TYPE1_OFFSET 14
#define TYPE2_OFFSET 16
#define OP1_OFFSET   18
#define OP2_OFFSET   25

/** Extern declarations **/
/* mnemonics table */
extern const struct InstrRecord instrLookup[];

/* instructions list */
extern char * instrBuffer; /* use this to write directly to file */
extern long instrPtr; /* points to next available (byte) space in the buffer */
extern long instrCap;

/** function prototypes **/
/* looking up instructions */
int getInstrInfo(const char * name, struct InstrRecord * out);
int isInstruction(const char * name);
int hasCustomOffset(struct Operand * op);

/* saving and writing instructions */
int saveInstruction(struct Instruction * instr);
int instructionSizeAgreement(struct Instruction * instr);
int instructionTypeAgreement(struct Instruction * instr);
int writeInstructions(FILE * stream);

#endif
