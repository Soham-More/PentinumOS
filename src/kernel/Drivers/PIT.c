#include "PIT.h"
 
#include <i686/IRQ.h>
#include <i686/PIC.h>

static uint32_t PIT_ticks_till_timeout = 0;
static bool PIT_timer_enabled = false;
static bool PIT_timedout = false;

void PIT_timer_irq(Registers* registers)
{
    if(!PIT_timer_enabled) return;

    PIT_ticks_till_timeout--;

    // ran out of time
    if(PIT_ticks_till_timeout == 0)
    {
        PIT_timer_enabled = false;
        PIT_timedout = true;
    }
}

void PIT_setTimeout(uint32_t timeOut)
{
    PIT_ticks_till_timeout = timeOut;
    PIT_timer_enabled = true;
    PIT_timedout = false;
}

void PIT_cancelTimeout()
{
    PIT_timer_enabled = false;
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