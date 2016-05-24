#ifndef LABELS_H
#define LABELS_H

/** symbol table infrastructure **/
typedef struct {
    char * label;
    int location;
} LabelRec;

/* for labels that are undefined, store in a list */
typedef struct {
    char * label;
    int * valueptr;
} UndefLabel;

void saveLabel(const char * label, int location);
int getLabelLocation(const char * label);

void saveUndefLabel(const char * label, int * valueptr);
void resolveLabels(void);

#endif
