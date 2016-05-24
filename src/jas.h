#ifndef JAS_H
#define JAS_H

/* default output file name */
#define DEFAULT_OUT "a.out"

/* masks for interpreting set flags */
#define OUT_FLAG 0x1
#define DEBUG_FLAG 0x2

/* optstring for use with getopt */
#define OPTS "ho:D"

/* definition of long options */
const struct option LOPTS[] = {
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

/* holds argument information to pass back to main */
struct argInfo {
    char flags;
    char * outfilename;
};

/* flex globals */
extern int yylex();
extern FILE * yyin;
extern int lineno;

/* fn prototypes */
static int parseArgs(int argc, char * const argv[], struct argInfo *);

#endif
