#pragma once

#include <includes.h>
#include "ISR.h"

typedef void (*IRQHandler)(Registers* registers);

void init_irq();
void IRQ_registerHandler(uint8_t irq, IRQHandler handler);
