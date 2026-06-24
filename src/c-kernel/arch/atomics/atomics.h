#pragma once

#include <includes.h>

// atomic operations for 32-bit integers

// atomically increments the value pointed to by ptr and returns the new value
u32 atomic_inc(u32* ptr);
// atomically decrements the value pointed to by ptr and returns the new value
u32 atomic_dec(u32* ptr);
// atomically adds val to the value pointed to by ptr and returns the new value
u32 atomic_add(u32* ptr, u32 val);
// atomically subtracts val from the value pointed to by ptr and returns the new value
u32 atomic_sub(u32* ptr, u32 val);



