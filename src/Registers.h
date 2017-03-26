#ifndef REGISTERS_H
#define REGISTERS_H

typedef unsigned char RegisterId;

int isShortRegister(char);
RegisterId getRegisterId(const char *);

#endif
