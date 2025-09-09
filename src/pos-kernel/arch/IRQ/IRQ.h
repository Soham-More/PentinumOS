#pragma once

#include <includes.h>
#include "../ISR/ISR.h"

typedef void (*IRQHandler)(Registers* registers);

void init_irq();
void IRQ_registerHandler(u8 irq, IRQHandler handler);
