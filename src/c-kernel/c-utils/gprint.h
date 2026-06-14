#pragma once
#include <includes.h>
#include <stdarg.h>

void gp_putc(char ch);

typedef struct {
    u8 error;
    u8 type;
    u8 bytesize;
    u8 sign;
    u8 align;
    u8 fill;
    usize width;
} gp_fmt_spec;
gp_fmt_spec gp_parse_fmt_spec(const char* fmt_spec, usize size);

void gp_printer(const char* fmt, usize len, va_list* vargs);
void gp_print(const char* fmt, ...);
