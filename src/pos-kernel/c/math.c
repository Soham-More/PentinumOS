#include "math.h"

uint64_t DivRoundUp(uint64_t p, uint64_t q)
{
    return (p + q - 1) / q;
}

uint64_t DivRoundDown(uint64_t p, uint64_t q)
{
    return p / q;
}

u32 RoundUpTo2Power(u32 v)
{
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

