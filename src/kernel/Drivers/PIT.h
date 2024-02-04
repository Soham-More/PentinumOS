#pragma once

#include <includes.h>

#define PIT_IRQ 0
#define PIT_FREQUENCY 100 // Hz
#define PIT_MAX_FREQ 1193182 // Hz

// set time in seconds
void PIT_setTimeout(uint32_t timeOut);
void PIT_cancelTimeout();

void PIT_sleep(uint32_t time);

bool PIT_hasTimedOut();

void PIT_init();
