#ifndef REGISTERS_H
#define REGISTERS_H

typedef unsigned char RegisterId;

int is_short_register(char);
RegisterId register_id(const char *);

#endif
