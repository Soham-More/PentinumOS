#pragma once

#include <includes.h>

#define PS2_ERROR 0xFC
#define PS2_DATA 0x60

void PS2_init();

u8 PS2_out(u8 port, u8 byte);
u8 PS2_in(u8 port);
