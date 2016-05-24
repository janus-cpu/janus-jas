#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>

#include "../src/Instruction.h"

#define START_FILE_STR "#ifndef INSTRUCTIONLIST_H\n#define INSTRUCTIONLIST_H\n" \
    "\n#include \"Instruction.h\"\n\n" \
    "\nconst InstrRecord instrLookup[] = {\n\n"
#define RECORD_STR "    {\"%s\",   0x%x,   IT_%c},\n"
#define SENTINEL_STR "    {NULL} /* sentinel */\n"
#define END_FILE_STR "\n};\n\n#endif"

#define MAX_INSTRS 0x200 /* 2^9 instructions */
#define INSTR_CFG_FILENAME "instructions.jascfg"
#define HEADER_NAME "InstructionList.h"
#define HEX_BASE 16

#define EXPECTED_ARGS 2

static InstrRecord instrInput[MAX_INSTRS];

int main(int argc, char *argv[]) {
    FILE * instrfile = fopen(INSTR_CFG_FILENAME, "rb");
    FILE * outfile;
    char * outfilename = HEADER_NAME;
    char buffer[BUFSIZ];

    if (argc > EXPECTED_ARGS) {
        fprintf(stderr, "Wrong number of input arguments.\n");
    }

    if (argc == EXPECTED_ARGS) {
        outfilename = argv[1];
    }

    /* open file */
    outfile = fopen(outfilename, "wb");

    /** begin writing to it **/
    fprintf(outfile, START_FILE_STR);

    while (fgets(buffer, BUFSIZ, instrfile) != NULL) {
        InstrRecord rec = {0};
        char * seek;
        char * name;
        int nameleng;
        enum OpCode opcode;
        enum InstructionType type;

        if (*buffer == ';' || *buffer == '\n') continue;

        /* mnemonic string */
        seek = strchr(buffer, '\t');
        nameleng = seek - buffer;
        name = (char *) malloc(nameleng + 1);
        strncpy(name, buffer, nameleng);
        name[nameleng] = '\0';

        /* opcode */
        errno = 0;
        opcode = (enum OpCode) strtol(seek + 1, &seek, HEX_BASE);

        /* instr type */
        switch (*(seek + 1)) {
            case 'N': type = IT_N; break;
            case 'A': type = IT_A; break;
            case 'B': type = IT_B; break;
            case 'X': type = IT_X; break;
            case 'I': type = IT_I; break;
            case 'P': type = IT_P; break;
            case 'U': type = IT_U; break;
            default : type = -1; /* should not happen */
        }

        fprintf(outfile, RECORD_STR, name, opcode, seek[1]);

        rec.name = name;
        rec.opcode = opcode;
        rec.type = type;

        instrInput[opcode] = rec;
    }

    fprintf(outfile, SENTINEL_STR);
    fprintf(outfile, END_FILE_STR);

    if (feof(instrfile)) fprintf(stderr, "Finished reading instrfile!\n");
    if (ferror(instrfile)) fprintf(stderr, "Error reading instrfile.\n");

    fclose(instrfile);
    fclose(outfile);
}
