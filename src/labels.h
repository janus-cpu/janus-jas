#ifndef LABELS_H
#define LABELS_H

/** symbol table infrastructure **/
typedef struct {
    char * label;
    int address; // Location that the label labels.
} LabelRec;

/* for labels that are undefined, store in a list */
typedef struct {
    char * label;
    int offset; // Offset from out_buffer where label is used.
} UndefLabel;

void save_label(const char * label);
int label_address(const char * label);

void save_undef_label(const char * label);
void resolve_labels(void);

#endif
