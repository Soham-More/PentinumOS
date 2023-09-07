#pragma once

#include <includes.h>

#define PIT_IRQ 0

// set time in seconds
void PIT_setTimeout(uint32_t timeOut);
void PIT_cancelTimeout();

void PIT_sleep(uint32_t time);

bool PIT_hasTimedOut();

void PIT_init();
