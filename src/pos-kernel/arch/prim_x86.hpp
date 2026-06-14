#pragma once

#include <includes.h>

#include <ISR/ISR.h>

// x86 C++ primitive types and functions
namespace x86
{
    struct registers_t
    {
        // x86 32-bit registers
        // in the reverse order they are pushed:
        u32 ds;                                            // data segment pushed by us
        u32 edi, esi, ebp, useless, ebx, edx, ecx, eax;    // pusha
        u32 eip, cs, eflags, esp, ss;                      // pushed automatically by CPU
    } _packed;

    ;
}
