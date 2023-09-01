#include "math.h"

uint64_t DivRoundUp(uint64_t p, uint64_t q)
{
    return (p + q - 1) / q;
}

uint64_t DivRoundDown(uint64_t p, uint64_t q)
{
    return p / q;
}
