#ifndef INSTRUCTIONLIST_H
#define INSTRUCTIONLIST_H

#include "Instruction.h"


const InstrRecord instrLookup[] = {

    {"NOP",   0x0,   IT_N},
    {"ADD",   0x1,   IT_A},
    {"ADC",   0x2,   IT_A},
    {"SUB",   0x3,   IT_A},
    {"SBB",   0x4,   IT_A},
    {"CMP",   0x5,   IT_A},
    {"CMP",   0x6,   IT_B},
    {"TEST",   0x7,   IT_A},
    {"TEST",   0x8,   IT_B},
    {"DEC",   0x9,   IT_P},
    {"INC",   0xa,   IT_P},
    {"NEG",   0xf,   IT_P},
    {"NOT",   0x10,   IT_P},
    {"AND",   0x11,   IT_A},
    {"OR",   0x12,   IT_A},
    {"XOR",   0x13,   IT_A},
    {"JMP",   0x30,   IT_U},
    {"CALL",   0x3c,   IT_U},
    {"RET",   0x3d,   IT_N},
    {"HLT",   0x3e,   IT_N},
    {"MOV",   0x50,   IT_A},
    {NULL} /* sentinel */

};

#endif