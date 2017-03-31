#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdio.h>
#include "instruction.h"

/*
 * Assemble input file to output file.
 */
void assemble(FILE * in, FILE * out);

/*
 * Write instruction to buffer.
 */
int save_instruction(struct Instruction * instr);
int write_instructions(FILE * stream, int size);

// Global variables
extern int loc_ctr;
extern unsigned char out_buffer[];

#endif
