#include "IDT.h"

#include "../GDT/GDT.h"

typedef struct 
{
    u16 offset_low;
    u16 kernel_cs;
    u8  reserved;
    u8  flags;
    u16 offset_high;
}_packed IDT_ENTRY;

typedef struct
{
    u16 limit;
    IDT_ENTRY* entries;
} _packed IDT_DESC;

static IDT_ENTRY IDT[256];

static IDT_DESC idt_desc = 
    {
        sizeof(IDT) - 1,
        &IDT[0]
    };

_import void _asmcall i686_load_idt(void* idt_desc);

void i686_set_idt_desc(u8 vector, void* handler, u16 code_segment, u8 flags)
{
    IDT_ENTRY* entry = &IDT[vector];

    entry->flags = flags;
    entry->kernel_cs = code_segment;
    entry->offset_low = ((u32)handler) & 0xFFFF;
    entry->offset_high = ((u32)handler) >> 16;
    entry->reserved = 0;
}

void i686_load_idt_current()
{
    i686_load_idt(&idt_desc);
}

void i686_enable_interrupt(u8 vector)
{
    IDT[vector].flags |= IDT_INTERRUPT_ENABLED;
}

void i686_disable_interrupt(u8 vector)
{
    IDT[vector].flags &= ~IDT_INTERRUPT_ENABLED;
}
