#pragma once

#include <includes.h>

enum IDT_FLAGS
{
    IDT_TASK_GATE            = 0x5,
    IDT_INTERRUPT_GATE_16BIT = 0x6,
    IDT_TRAP_GATE_16BIT      = 0x7,
    IDT_INTERRUPT_GATE_32BIT = 0xE,
    IDT_TRAP_GATE_32BIT      = 0xF,

    IDT_PRIVILEGE_RING0      = (0 << 5),
    IDT_PRIVILEGE_RING1      = (1 << 5),
    IDT_PRIVILEGE_RING2      = (2 << 5),
    IDT_PRIVILEGE_RING3      = (3 << 5),

    IDT_INTERRUPT_ENABLED = 0x80,
};

void i686_set_idt_desc(uint8_t vector, void* handler, uint16_t code_segment, uint8_t flags);

void i686_load_idt_current();

void i686_enable_interrupt(uint8_t vector);
void i686_disable_interrupt(uint8_t vector);
