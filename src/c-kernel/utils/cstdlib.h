#pragma once

#include <includes.h>

// get the length of an array
#define _LEN(x) (sizeof(x)/sizeof(x[0]))
#define _BYTELEN(x) (sizeof(x))

// cstring helpers
size_t strlen(const char* a);
bool strcmp(const char* a, const char* b, usize length);

// base memory manip
void* memcpy(void* dst, const void* src, usize bytelength);
void  memset(u8* target, usize bytelength, u8 value);

// math helpers
u64 div_ceil(u64 p, u64 q);
u64 div_floor(u64 p, u64 q);
u32 ceil_to_2power(u32 v);

#define MIN(a, b) ((a) < (b) ? (a) : (b))

