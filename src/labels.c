#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "debug.h"
#include "instruction.h"
#include "labels.h"
#include "output.h"

// symbol tables
static LabelRec * sym_tab;
static UndefLabel * undef_labels;
static int numlabels = 0;
static int numundef = 0;

static void print_unresolved(const char * label) {
    fprintf(stderr, "error: Unresolved label `%s`\n", label);
}

/*
 * TODO: this needs to work across large files bigger than BUFSIZ bytes
 */
void resolve_labels(void) {
    int index, value;
    for (index = 0; index < numundef; index++) {
        UndefLabel undef = undef_labels[index];
        value = label_address(undef.label);

        /* resolve dat label */
        memcpy(out_buffer + undef.offset, &value, sizeof(int));

        if (value == -1) {
            print_unresolved(undef.label);
            continue;
        }

        free(undef.label); /* no need for this string anymore */
    }
    free(undef_labels); /* no need for this list anymore */
}

/*
 * Save label with loc_ctr's address.
 */
void save_label(const char * label) {
    LabelRec rec; // New record to place in record table;
    int llen = strlen(label) + 1; // For alloc'ing and copying -- includes nul.
    char * labelcpy = malloc(llen); // For saving string.

    // Allocate more space for table.
    LabelRec * temp = realloc(sym_tab, sizeof(LabelRec) * (numlabels + 1));
    if (temp == NULL) { fprintf(stderr, "realloc() error.\n"); return; }
    sym_tab = temp;

    /* load new label insert into table */
    rec.label = strncpy(labelcpy, label, llen);
    rec.address = loc_ctr;
    sym_tab[numlabels++] = rec;

    DEBUG(" + sym_tab[%d] Inserted `%s`, location %d",
            numlabels - 1, rec.label, rec.address);
}

/*
 * Save a label that hasn't been defined yet.
 */
void save_undef_label(const char * label) {
    UndefLabel rec;
    int llen = strlen(label) + 1; // For alloc'ing and copying -- includes nul.
    char * labelcpy = malloc(llen);

    strncpy(labelcpy, label, llen);
    labelcpy[llen - 1] = '\0'; /* null terminate */

    /* populate entry */
    rec.label = labelcpy;
    rec.offset = loc_ctr; // XXX find the actual offset

    // Make more space for new label.
    UndefLabel * temp = realloc(undef_labels, sizeof(UndefLabel) * (numundef + 1));
    if (temp == NULL) { fprintf(stderr, "realloc() error.\n"); return; }
    undef_labels = temp;

    undef_labels[numundef++] = rec;

    DEBUG(" + undef_labels[%d] Inserted `%s`",
            numundef - 1, rec.label);
}

/*
 * Find the label's address
 */
int label_address(const char * label) {
    for (int i = 0; i < numlabels; i++) {
        if (0 == strcmp(sym_tab[i].label, label)) {
            return sym_tab[i].address;
        }
    }

    return -1;
}
