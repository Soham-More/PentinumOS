#pragma once
#include <includes.h>
#include <stdarg.h>

#define STDIO_Video 0
#define STDIO_E9 1
#define STDERR 2

void clrscr();
void clrline();

void putc(char ch);
void puts(const char* str);

// 's': print a string till a null charecter
// 'S': print a string with given size
void vprintf(const char* fmt, va_list vargs);

// 's': print a string till a null charecter
// 'S': print a string with given size
void printf(const char* fmt, ...);

char getch();
char getchar();

void set_ostream(uint8_t out);
