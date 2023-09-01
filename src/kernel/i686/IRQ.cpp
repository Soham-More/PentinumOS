#include "IRQ.h"
#include "PIC.h"
#include "io.h"
#include <std/logger.h>

IRQHandler irq_handlers[16];

void _default_irq_handler(Registers* registers)
{
    int irq = registers->interrupt - REMAP_PIC_OFFSET;

    uint16_t pic_isr = PIC_get_isr();
    uint16_t pic_irr = PIC_get_irr();

    if(irq_handlers[irq] != nullptr)
    {
        irq_handlers[irq](registers);
    }
    else
    {
        log_warn("Unhandled IRQ Caught: IRQ #%d (ISR = %x, IRR = %x)\n", irq, pic_isr, pic_irr);
    }

    PIC_EOI(irq);
}

void init_irq()
{
    init_pic();

    for(uint8_t vector = 0; vector < 16; vector++)
    {
        i686_set_isr(vector + REMAP_PIC_OFFSET, _default_irq_handler);
    }

    i686_EnableInterrupts();
}

void IRQ_registerHandler(uint8_t irq, IRQHandler handler)
{
    if(irq < 16)
    {
        irq_handlers[irq] = handler;
    }
}
