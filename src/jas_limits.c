#include "jas_limits.h"

/*
 * Give tightest [1,2,4] size in bytes that can fit value.
 * Returns -1 if can't fit any size.
 */
int fit_size(unsigned long value) {
    // If value is negative and can fit in a word, use a word.
    if ((long) value < 0 && (long) value == (int32_t) value) {
        return WORD_WIDTH;
    }

    if (value <= UBYTE_MAX)
        return BYTE_WIDTH;
    if (value <= UHALF_MAX)
        return HALF_WIDTH;
    if (value <= UWORD_MAX)
        return WORD_WIDTH;

    // Too big!
    return -1;
}

/*
 * Fit size with a hint that it's possible to widen. No hint if hint == 0.
 * Returns -1 if value can't fit any size.
 */
int fit_size_hint(unsigned long value, int hint) {
    int size = fit_size(value);
    if (hint == 0) return size;
    if (size == -1) return -1;
    return size <= hint ? hint : -1;
}
