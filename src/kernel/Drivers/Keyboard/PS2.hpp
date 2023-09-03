#pragma once

#include <includes.h>

#define PS2_ERROR 0xFC
#define PS2_DATA 0x60

void PS2_init();

uint8_t PS2_out(uint8_t port, uint8_t byte);
uint8_t PS2_in(uint8_t port);
