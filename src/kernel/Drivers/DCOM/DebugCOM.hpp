#pragma once

#include <includes.h>
#include <i686/x86.h>

namespace DCOM
{
    inline void putc(char c)
    {
        x86_outb(0xE9, c);
    }
}
