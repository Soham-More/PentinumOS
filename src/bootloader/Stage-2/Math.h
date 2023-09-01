#pragma once
#include "includes.h"

#define swap(x, y, type) { type __SWAP_TEMP = x; x = y; y = __SWAP_TEMP; }

uint32_t min(uint32_t x, uint32_t y);
uint32_t max(uint32_t x, uint32_t y);
