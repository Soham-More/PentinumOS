#pragma once
#include <includes.h>
#include <stdarg.h>

void clrscr();

void flush();

void putc(char ch);
void puts(const char* str);

// 's': print a string till a null charecter
// 'S': print a string with given size
void vprintf(const char* fmt, va_list vargs);

// 's': print a string till a null charecter
// 'S': print a string with given size
void printf(const char* fmt, ...);
