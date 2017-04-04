#ifndef LABELS_H
#define LABELS_H

/** symbol table infrastructure **/
typedef struct {
    char * label; // Label name
    int address;  // Either the address the label points to (sym_tab),
                  // or the address of the label in memory (undef_tab).
} SymbolRec;

typedef struct {
    SymbolRec * table;
    int size; // Number of elements
    int cap;  // Maximum capacity
} SymbolTable;

void save_label(const char * label);
int label_address(const char * label);

void save_undef_label(const char * label, int offset);
void resolve_labels(void);

#endif
