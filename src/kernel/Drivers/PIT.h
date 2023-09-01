#pragma once

#include <includes.h>

#define PIT_IRQ 0

void PIT_setTimeout(uint32_t timeOut);
void PIT_cancelTimeout();

bool PIT_hasTimedOut();

void PIT_init();
