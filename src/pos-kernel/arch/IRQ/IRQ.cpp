#include "IRQ.h"
#include "PIC.h"
#include "../x86.h"
#include <io/io.h>

IRQHandler irq_handlers[16];

void _no_stack_trace _default_irq_handler(Registers* registers)
{
    int irq = registers->interrupt - REMAP_PIC_OFFSET;

    u16 pic_isr = PIC_get_isr();
    u16 pic_irr = PIC_get_irr();

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

    for(u8 vector = 0; vector < 16; vector++)
    {
        i686_set_isr(vector + REMAP_PIC_OFFSET, _default_irq_handler);
    }

    x86_EnableInterrupts();
}

void IRQ_registerHandler(u8 irq, IRQHandler handler)
{
    if(irq < 16)
    {
        irq_handlers[irq] = handler;
    }
}
