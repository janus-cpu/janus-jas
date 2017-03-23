#ifndef INSTRUCTIONLIST_H
#define INSTRUCTIONLIST_H

#include "Instruction.h"


const struct InstrRecord instrLookup[] = {

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
    {"JE",   0x31,   IT_U},
    {"JZ",   0x31,   IT_U},
    {"JNE",   0x32,   IT_U},
    {"JNZ",   0x32,   IT_U},
    {"JL",   0x33,   IT_U},
    {"JLE",   0x34,   IT_U},
    {"JG",   0x35,   IT_U},
    {"JGE",   0x36,   IT_U},
    {"JLU",   0x37,   IT_U},
    {"JLEU",   0x38,   IT_U},
    {"JGU",   0x39,   IT_U},
    {"JGEU",   0x3a,   IT_U},
    {"INT",    0x3b,   IT_T},
    {"CALL",   0x3c,   IT_U},
    {"RET",   0x3d,   IT_N},
    {"HLT",   0x3e,   IT_N},
    {"IRET",   0x3f,   IT_N},
    {"LOM",   0x40,   IT_U},
    {"ROM",   0x41,   IT_P},
    {"LOI",   0x42,   IT_U},
    {"ROI",   0x43,   IT_P},
    {"ROP",   0x44,   IT_P},
    {"LFL",   0x45,   IT_U},
    {"RFL",   0x46,   IT_P},
    {"MOV",   0x50,   IT_A},
    {"POP",   0x51,   IT_P},
    {"PUSH",   0x52,   IT_U},
    {"IN",   0x53,   IT_I},
    {"OUT",   0x54,   IT_I},
    {"XCHG",   0x55,   IT_X},
    {NULL} /* sentinel */

};

#endif
