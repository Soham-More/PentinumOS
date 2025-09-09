#pragma once

#include <includes.h>
#include <arch/x86.h>

#define STD_EXCEPTION_BASE 0x0
#define STD_EXCEPTION_SIZE 0x10
#define NO_HEAP_MEMORY 0x10
#define USB_ERROR      0x11

namespace cpu
{
    inline void halt() { __asm__("hlt"); }

    void initialize_exceptions();
}


