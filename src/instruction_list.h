#ifndef INSTRUCTIONLIST_H
#define INSTRUCTIONLIST_H

#include "instruction.h"

const struct InstrRecord instr_lookup[] = {

    {"ADD",   0x0,   IT_A},
    {"SUB",   0x2,   IT_A},
    {"ADC",   0x4,   IT_A},
    {"SBB",   0x6,   IT_A},
    {"RSUB",  0x8,   IT_A},
    {"NOR",   0x20,  IT_A},
    {"NAND",  0x24,  IT_A},
    {"OR",    0x28,  IT_A},
    {"ORN",   0x2A,  IT_A},
    {"AND",   0x2C,  IT_A},
    {"ANDN",  0x2E,  IT_A},
    {"MOV",   0x30,  IT_A},
    {"XNOR",  0x34,  IT_A},
    {"NOT",   0x38,  IT_P},
    {"XOR",   0x3C,  IT_A},
    {"CMP",   0x42,  IT_A},
    {"TEST",  0x6C,  IT_A},

    {"JMP",   0x80,  IT_U},
    {"JE",    0x81,  IT_U},
    {"JZ",    0x81,  IT_U},
    {"JNE",   0x82,  IT_U},
    {"JNZ",   0x82,  IT_U},
    {"JL",    0x83,  IT_U},
    {"JLE",   0x84,  IT_U},
    {"JG",    0x85,  IT_U},
    {"JGE",   0x86,  IT_U},
    {"JLU",   0x87,  IT_U},
    {"JLEU",  0x88,  IT_U},
    {"JGU",   0x89,  IT_U},
    {"JGEU",  0x8A,  IT_U},

    {"CALL",  0x8B,  IT_U},
    {"RET",   0x8C,  IT_N},
    {"HLT",   0x8D,  IT_N},
    {"INT",   0x8E,  IT_T},
    {"IRET",  0x8F,  IT_N},

    {"LOM",   0x70,  IT_U},
    {"ROM",   0x71,  IT_P},
    {"LOI",   0x72,  IT_U},
    {"ROI",   0x73,  IT_P},
    {"ROP",   0x75,  IT_P},
    {"LFL",   0x76,  IT_U},
    {"RFL",   0x77,  IT_P},
    {"LOT",   0x78,  IT_U},
    {"ROT",   0x79,  IT_P},
    {"LOS",   0x7A,  IT_U},
    {"ROS",   0x7B,  IT_P},
    {"LOF",   0x7C,  IT_U},
    {"ROF",   0x7D,  IT_P},

    {"POP",   0xA0,  IT_P},
    {"PUSH",  0xA2,  IT_U},
    {"IN",    0xA4,  IT_I},
    {"OUT",   0xA6,  IT_I},
    {"XCHG",  0xA8,  IT_X},
    {"POPR",  0xAA,  IT_N},
    {"PUSHR", 0xAB,  IT_N},

    {"MOVE",   0xB0,  IT_A},
    {"MOVZ",   0xB0,  IT_A},
    {"MOVNE",  0x82,  IT_A},
    {"MOVNZ",  0x82,  IT_A},
    {"MOVL",   0x84,  IT_A},
    {"MOVLE",  0x86,  IT_A},
    {"MOVG",   0x88,  IT_A},
    {"MOVGE",  0x8A,  IT_A},
    {"MOVLU",  0x8C,  IT_A},
    {"MOVLEU", 0x8E,  IT_A},
    {"MOVGU",  0x80,  IT_A},
    {"MOVGEU", 0x82,  IT_A},

    // Synthetic Instructions
    {"NOP",   0xFF,  IT_N},
    {"INC",   0xFF,  IT_P},
    {"DEC",   0xFF,  IT_P},
    {"NEG",   0xFF,  IT_P},
    {"CLR",   0xFF,  IT_P},

    {NULL} /* sentinel */

};

#endif
