#include "output.h"

#include <string.h> // For memcpy()
#include <stdlib.h>

#include "debug.h" // For DEBUG()

#include "parser.h"
#include "lexer.h"

unsigned char out_buffer[BUFSIZ]; // Buffer of data to write to file.
static int buffers; // Used as offset for each new out_buffer used.
int loc_ctr;        // Location counter, for label resolution.
static FILE * outfile;

/** entry function to begin assembling **/
void assemble(FILE * in, FILE * out) {
    // Let the lexer know about the infile.
    lexfile = in;
    outfile = out;

    parse(); // initial parsing, label recognition, type saving and syntax checks

    analyze(); // label resolution and type analysis

    /* prevent writing to file if there were errors */
    if (!j_err) {
        /* write instructions to outfile */
        write_instructions(out, loc_ctr);
    }
}

// Convert byte count into proper bit encoding
static int size_as_bits(enum ConstantSize sz) {
    if (sz == CS_WORD)
        return 3;
    else
        return sz;
}

/*
 * Write operand to buffer.
 * Side effects: increments loc_ctr by number of bytes written.
 */
static void save_operand(struct Operand * op) {
    unsigned char desc = 0;    // Descriptor byte.
    unsigned char extra = 0;   // Extra descriptor may be needed.
    int size_bits = size_as_bits(op->const_size); // Constant size in bytes.

    // Write down the C/R:D/I type.
    desc |= op->type;

    // Write down all the rest of the descriptor byte + subsequent bytes.
    switch (op->type) {
        case OT_CONST:
        case OT_IND:
            // Set size of constant and register val.
            desc |= size_bits << 2;
            desc |= op->reg << 4;
            break;

        case OT_REG:
            // Set register.
            desc |= op->reg << 2;
            break;

        case OT_SC_IND:
            // Set descriptor byte and
            desc |= op->scale << 2;
            desc |= op->reg << 4;
            extra |= op->index_reg;
            extra |= size_bits << 4;
            break;
    }

    // Write descriptor.
    out_buffer[loc_ctr++] = desc;

    // Write out extra descriptor for scaled index.
    if (op->type == OT_SC_IND) {
        out_buffer[loc_ctr++] = extra;
    }

    // Write out constant if it isn't skipped.
    if (op->const_size != 0) {
        memcpy(&out_buffer[loc_ctr], &op->constant, op->const_size);
        loc_ctr += op->const_size;
    }

    // Debugging stuff
    if (debug_on) {
        if (op->type == OT_CONST) {
            DEBUG("\tOT_CONST  sz=%d cs=%d ct=%d",
                     op->size, op->const_size, op->constant);
        } else if (op->type == OT_IND) {
            DEBUG("\tOT_IND    sz=%d rg=%d cs=%d ct=%d",
                     op->size, op->reg, op->const_size, op->constant);
        } else if (op->type == OT_REG) {
            DEBUG("\tOT_REG    sz=%d rg=%d",
                     op->size, op->reg);
        } else {
            DEBUG("\tOT_SC_IND sz=%d rg=%d sc=%d irg=%d cs=%d ct=%d",
                     op->size, op->reg, op->scale, op->index_reg,
                     op->const_size, op->constant);
        }
    }
}

/*
 * Write instruction to buffer.
 */
int save_instruction(struct Instruction * instr) {
    // Write out instructions if buffer is full.
    // TODO: for later, will not work for now
    if (loc_ctr + MAX_INSTR_SIZE >= BUFSIZ) {
        write_instructions(outfile, BUFSIZ);
        buffers++;
    }

    // Lay in the opcode.
    out_buffer[loc_ctr++] = instr->opcode;

    DEBUG("<< Output: Writing instr 0x%x (%s) with",
          instr->opcode, instr->name);

    // INT doesn't have a descriptor byte.
    if (instr->opcode == OPCODE_INT) {
        out_buffer[loc_ctr++] = instr->op1.constant;
        DEBUG("%s","");
        return EXIT_SUCCESS;
    }

    // Write operands, if instruction has them.
    if (has_operands(instr->type))
        save_operand(&instr->op1);

    if (has_two_operands(instr->type))
        save_operand(&instr->op2);

    DEBUG("%s","");

    return EXIT_SUCCESS;
}

int write_instructions(FILE * stream, int size) {
    fwrite(out_buffer, sizeof(char), size, stream);
    return EXIT_SUCCESS; /* TODO: may not be successful? */
}
