#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "debug.h"
#include "Instruction.h"
#include "Labels.h"

/* symbol table */
static LabelRec * symTab;
static UndefLabel * undefLabels;
static long numlabels = 0;
static int numundef = 0;

static void printUnresolved(const char * label) {
    fprintf(stderr, "error: Unresolved label `%s'\n", label);
}

void resolveLabels(void) {
    int index, value;
    for (index = 0; index < numundef; index++) {
        UndefLabel undef = undefLabels[index];
        value = getLabelLocation(undef.label);

        /* resolve dat label */
        memcpy(instrBuffer + undef.valueptr, &value, sizeof(value));

        if (value < 0) {
            printUnresolved(undef.label);
            continue;
        }

        free(undef.label); /* no need for this string anymore */
    }
    free(undefLabels); /* no need for this list anymore */
}

void saveLabel(const char * label, int location) {
    LabelRec newRec;
    LabelRec * temp;
    char * labelcpy = (char *) malloc(strlen(label));

    /* allocate more space for table */
    temp = (LabelRec *) realloc(symTab, sizeof(LabelRec) * (numlabels + 1));
    if (temp == NULL) { fprintf(stderr, "realloc() error.\n"); return; }
    symTab = temp;

    /* load new label, remove `:' , insert into table */
    newRec.label = strncpy(labelcpy, label, strlen(label));
    newRec.label[strlen(label) - 1] = '\0'; /* null terminate */
    newRec.location = location;
    symTab[numlabels++] = newRec;

    DEBUG("Symtab[%ld] Inserted `%s', location %d",
            numlabels - 1, newRec.label, newRec.location);
}

void saveUndefLabel(const char * label, long valueptr) {
    UndefLabel newLabel;
    UndefLabel * temp;
    char * labelcpy = (char *) malloc(strlen(label));

    strncpy(labelcpy, label, strlen(label));
    labelcpy[strlen(label)] = '\0'; /* null terminate */

    /* populate entry */
    newLabel.label = labelcpy;
    newLabel.valueptr = valueptr;

    temp = (UndefLabel *) realloc(undefLabels, sizeof(UndefLabel) * (numundef + 1));
    if (temp == NULL) { fprintf(stderr, "realloc() error.\n"); return; }
    undefLabels = temp;

    undefLabels[numundef++] = newLabel;

    DEBUG("undefLabels[%d] Inserted `%s'",
            numundef - 1, newLabel.label);
}

int getLabelLocation(const char * label) {
    /* search the array pls */
    int i;
    for (i = 0; i < numlabels; i++) {
        if (0 == strcmp(symTab[i].label, label)) {
            return symTab[i].location;
        }
    }

    return -1;
}
