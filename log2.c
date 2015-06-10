#include "log2.h"

unsigned int
htable_log2(unsigned int n)
{
    unsigned int how_many_bits = 1, threshold = 2;
    while (n > threshold) {
        how_many_bits++;
        threshold <<= 1;
    }
    
    return how_many_bits;
}
