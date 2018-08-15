#include "util.h"

void SwapWords(Word *words, int numWords)
{
    int i;

    for (i = 0; i < numWords; i++) {
        Word value = words[i];

        value =
            ((value >> 24) & 0x000000FF) |
            ((value >> 8) & 0x0000FF00) |
            ((value << 8) & 0x00FF0000) |
            ((value << 24) & 0xFF000000);

        words[i] = value;
    }
}

void SwapHalfword(Halfword *halfWord)
{
    *halfWord =
        ((*halfWord >> 8) & 0x00FF) |
        ((*halfWord << 8) & 0xFF00);
}
