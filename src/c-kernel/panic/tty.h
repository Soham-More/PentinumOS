#pragma once
#include <includes.h>
#include <stdarg.h>

void tty_panic_vprintf(const char* fmt, va_list vargs);
void tty_panic_printf(const char* fmt, ...);

