#pragma once

#include <includes.h>

typedef struct REGISTERS
{
    // in the reverse order they are pushed:
    u32 ds;                                            // data segment pushed by us
    u32 edi, esi, ebp, useless, ebx, edx, ecx, eax;    // pusha
    u32 interrupt, error;                              // we push interrupt, error is pushed automatically (or our dummy)
    u32 eip, cs, eflags, esp, ss;                      // pushed automatically by CPU
}_packed Registers;

typedef void (*ISRHandler)(Registers* registers);

void i686_init_isr();

void _no_stack_trace i686_dump_registers(Registers* registers);

void i686_set_isr(u8 isr_vector, ISRHandler isr);
