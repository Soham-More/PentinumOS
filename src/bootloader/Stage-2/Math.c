#include "Math.h"

uint32_t min(uint32_t x, uint32_t y)
{
    if(x > y)
    {
        return y;
    }

    return x;
}

uint32_t max(uint32_t x, uint32_t y)
{
    if(x > y)
    {
        return x;
    }
    
    return y;
}
