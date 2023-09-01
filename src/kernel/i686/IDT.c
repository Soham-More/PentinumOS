#include "IDT.h"

#include "GDT.h"

typedef struct 
{
    uint16_t offset_low;
    uint16_t kernel_cs;
    uint8_t  reserved;
    uint8_t  flags;
    uint16_t offset_high;
}_packed IDT_ENTRY;

typedef struct
{
    uint16_t limit;
    IDT_ENTRY* entries;
} _packed IDT_DESC;

static IDT_ENTRY IDT[256];

static IDT_DESC idt_desc = 
    {
        sizeof(IDT) - 1,
        &IDT[0]
    };

_import void _asmcall i686_load_idt(void* idt_desc);

void i686_set_idt_desc(uint8_t vector, void* handler, uint16_t code_segment, uint8_t flags)
{
    IDT_ENTRY* entry = &IDT[vector];

    entry->flags = flags;
    entry->kernel_cs = code_segment;
    entry->offset_low = ((uint32_t)handler) & 0xFFFF;
    entry->offset_high = ((uint32_t)handler) >> 16;
    entry->reserved = 0;
}

void i686_load_idt_current()
{
    i686_load_idt(&idt_desc);
}

void i686_enable_interrupt(uint8_t vector)
{
    IDT[vector].flags |= IDT_INTERRUPT_ENABLED;
}

void i686_disable_interrupt(uint8_t vector)
{
    IDT[vector].flags &= ~IDT_INTERRUPT_ENABLED;
}
