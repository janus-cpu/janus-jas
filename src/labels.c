#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "debug.h"
#include "instruction.h"
#include "labels.h"
#include "output.h"

// symbol tables
static SymbolTable sym_tab;
static SymbolTable undef_tab;

static void print_unresolved(const char * label) {
    fprintf(stderr, "error: Unresolved label `%s`\n", label);
}

/*
 * Append to a symbol table.
 * sym - the symbol table to append to
 * label - name of label to append
 * address - address referencing the label
 */
static void append_table(SymbolTable * sym, const char * label, int address) {
    // If needed, allocate more space for table by doubling the size.
    if (sym->size == sym->cap) {
        // Empty tables start with default space, but double in size later.
        if (sym->cap == 0)
            sym->cap = 16;
        else
            sym->cap *= 2;

        SymbolRec * temp = realloc(sym->table, sizeof(SymbolRec) * sym->cap);

        // Check for alloc errors
        if (temp == NULL) {
            fprintf(stderr, "realloc() error.\n");
            for (int i = 0; i < sym->size; i++) {
                if (sym->table[i].label != NULL) free(sym->table[i].label);
            }
            free(sym->table);
            return;
        }

        // Space alloc'd
        sym->table = temp;
    }

    // Set new label record into table.
    SymbolRec rec;
    int llen = strlen(label) + 1; // For alloc'ing and copying -- includes nul.
    char * labelcpy = malloc(llen); // For saving string.

    rec.label = strncpy(labelcpy, label, llen);
    rec.address = address;
    sym->table[sym->size++] = rec;
}

/*
 * Frees all memory associated with the symbol table.
 */
static void free_table(SymbolTable * sym) {
    // Free each entry's char* and whole table.
    for (int i = 0; i < sym->size; i++) {
        if (sym->table[i].label != NULL) free(sym->table[i].label);
    }
    free(sym->table);

    // Clear out everything to zero.
    sym->table = NULL;
    sym->size = sym->cap = 0;
}

/*
 * TODO: this needs to work across large files bigger than BUFSIZ bytes
 */
void resolve_labels(void) {
    for (int i = 0; i < undef_tab.size; i++) {
        SymbolRec * undef = &undef_tab.table[i];
        int value = label_address(undef->label);

        // Fill empty label spot.
        memcpy(out_buffer + undef->address, &value, sizeof(int));

        if (value == -1) {
            print_unresolved(undef->label);
            continue;
        }

    }

    free_table(&undef_tab);
}

/*
 * Save label with loc_ctr's address.
 */
void save_label(const char * label) {
    // Label has just been encountered, set address to loc_ctr.
    append_table(&sym_tab, label, loc_ctr);

    DEBUG(" + sym_tab[%d] Inserted `%s`, address %d",
            sym_tab.size - 1, label, loc_ctr);
}

/*
 * Save a label that hasn't been defined yet.
 * offset - Offset in bytes from the loc_ctr that the label will reside in
 */
void save_undef_label(const char * label, int offset) {
    append_table(&undef_tab, label, loc_ctr + offset);

    DEBUG(" + undef_labels[%d] Inserted `%s`",
            undef_tab.size - 1, label);
}

/*
 * Find the label's address in sym_tab.
 */
int label_address(const char * label) {
    for (int i = 0; i < sym_tab.size; i++) {
        if (0 == strcmp(sym_tab.table[i].label, label)) {
            return sym_tab.table[i].address;
        }
    }

    return -1;
}
