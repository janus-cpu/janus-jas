
#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <getopt.h>

#include "output.h"
#include "jas_strings.h"

#include "debug.h"
#include "jas.h"

#define OUTSET(s) ((s).flags & OUT_FLAG)
#define DEBUGSET(s) ((s).flags & DEBUG_FLAG)

/* definition of debug flag */
bool debug_on = false;
char * infilename = "(stdin)"; /* default name is stdin */

int main(int argc, char *argv[]) {
    FILE * infile;  /* input and output streams */
    FILE * outfile;

    char ** infilenameptr; /* input and output filenames */
    char * outfilename = DEFAULT_OUT;

    struct argInfo info = {0}; /* zero-fill struct for argument info */

    /* interpret command line arguments */
    parseArgs(argc, (char* const*) argv, &info);

    if (OUTSET(info)) outfilename = info.outfilename;
    if (DEBUGSET(info)) {
        debug_on = true;
    }

    DEBUG("Debugging set.");

    /* parseArgs will have permuted the argv array, optind
     * points to the first element of non-options */
    outfile = fopen(outfilename, "w");

    infilenameptr = argv + optind;
    if (optind == argc) /* empty file name */
        infile = stdin;
    else {
        infilename = *infilenameptr;
        infile = fopen(infilename, "rb"); /* TODO: this will eventually
                                             loop to include more files!! */
    }

    if (infile == NULL) {
        fprintf(stderr, STR_FILE_ERR, argv[optind]);
        return EXIT_FAILURE;
    }

    /* pass in assembly */
    assemble(infile, outfile);

    /* free memory */
    free(info.outfilename);

    /* close files */
    if (infile != stdin) fclose(infile);

    fflush(outfile);
    fclose(outfile);

    return EXIT_SUCCESS;
}

static int parseArgs(int argc, char * const argv[], struct argInfo * info) {
    int optret;

    while (-1 != (optret = getopt_long(argc, argv, OPTS, LOPTS, NULL))) {
        switch (optret) {
            case 'h': {
                printf(STR_USAGE, argv[0]);
                exit(0);
                break;
            }

            case 'o': {
                char * outfilename = (char *) malloc(strlen(optarg));
                strcpy(outfilename, optarg);

                info->flags |= OUT_FLAG;
                info->outfilename = outfilename;
                break;
            }

            case 'D': {
                info->flags |= DEBUG_FLAG;
                break;
            }

            case '?': {
                break;
            }

            case ':': {
                break;
            }

            default: {
                break;
            }
        }
    }

    return EXIT_SUCCESS;
}
