/*
 * Min and max values for JAS data widths.
 */

#ifndef JAS_LIMITS_H
#define JAS_LIMITS_H

#include <stdint.h>

#define SBYTE_MIN (int8_t) 0x80
#define SBYTE_MAX (int8_t) 0x7F
#define UBYTE_MAX (uint8_t) 0xFF
#define BYTE_WIDTH 1

#define SHALF_MIN (int16_t) 0x8000
#define SHALF_MAX (int16_t) 0x7FFF
#define UHALF_MAX (uint16_t) 0xFFFF
#define HALF_WIDTH 2

#define SWORD_MIN (int32_t) 0x80000000
#define SWORD_MAX (int32_t) 0x7FFFFFFF
#define UWORD_MAX (uint32_t) 0xFFFFFFFF
#define WORD_WIDTH 4

int fit_size(unsigned long value);
int fit_size_hint(unsigned long value, int hint);

#endif // JAS_LIMITS_H
