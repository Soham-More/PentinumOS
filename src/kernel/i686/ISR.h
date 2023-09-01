#pragma once

#include <includes.h>

typedef struct REGISTERS
{
    // in the reverse order they are pushed:
    uint32_t ds;                                            // data segment pushed by us
    uint32_t edi, esi, ebp, useless, ebx, edx, ecx, eax;    // pusha
    uint32_t interrupt, error;                              // we push interrupt, error is pushed automatically (or our dummy)
    uint32_t eip, cs, eflags, esp, ss;                      // pushed automatically by CPU
}_packed Registers;

typedef void (*ISRHandler)(Registers* registers);

void i686_init_isr();

void i686_dump_registers(Registers* registers);

void i686_set_isr(uint8_t isr_vector, ISRHandler isr);
