#pragma once
#include <stdint.h>
#include <stdbool.h>

void clrscr();

void putc(char ch);
void puts(const char* str);

void putHex(uint32_t byte);

// 's': print a string till a null charecter
// 'S': print a string with given size
void printf(const char* fmt, ...);
