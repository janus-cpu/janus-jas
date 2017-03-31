#include "jas_limits.h"

/*
 * Give tightest [1,2,4] size in bytes that can fit value.
 * Returns -1 if can't fit any size.
 */
int fit_size(int value) {
    if (SBYTE_MIN <= value && value <= UBYTE_MAX)
        return BYTE_WIDTH;
    if (SHALF_MIN <= value && value <= UHALF_MAX)
        return HALF_WIDTH;
    if (SWORD_MIN <= value && value <= UWORD_MAX)
        return WORD_WIDTH;

    // Too big!
    return -1;
}
