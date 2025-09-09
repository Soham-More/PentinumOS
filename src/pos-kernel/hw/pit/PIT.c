#include "PIT.h"
 
#include <arch/IRQ/IRQ.h>
#include <arch/IRQ/PIC.h>
#include <arch/x86.h>
#include <io/io.h>

static volatile u32 PIT_ticks_till_timeout = 0;
static volatile bool PIT_timer_enabled = false;
static volatile bool PIT_timedout = false;

void _no_stack_trace PIT_timer_irq(Registers* registers)
{
    if(!PIT_timer_enabled) return;

    // ran out of time
    if(PIT_ticks_till_timeout == 0)
    {
        PIT_timer_enabled = false;
        PIT_timedout = true;
    }

    PIT_ticks_till_timeout--;
}

void PIT_setTimeout(u32 timeOut)
{
    PIT_ticks_till_timeout = timeOut * 18;
    PIT_timer_enabled = true;
    PIT_timedout = false;
}

void PIT_cancelTimeout()
{
    PIT_timer_enabled = false;
}

void PIT_sleep(u32 time)
{
    PIT_setTimeout(time);

    while(!PIT_timedout);
}

bool PIT_hasTimedOut()
{
    return PIT_timedout;
}

void PIT_init()
{
    PIC_irq_unmask(0);

    IRQ_registerHandler(PIT_IRQ, PIT_timer_irq);
}
