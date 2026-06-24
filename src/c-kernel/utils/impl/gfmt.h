#pragma once
#include <includes.h>
#include <stdarg.h>

void gfmt_putc(void* ctx, char ch);

typedef struct {
    u8 error;
    u8 type;
    u8 bytesize;
    u8 sign;
    u8 align;
    u8 fill;
    usize width;
} gfmt_fmt_spec;
gfmt_fmt_spec gfmt_parse_fmt_spec(const char* fmt_spec, usize size);

void gfmt_printer(void* ctx, const char* fmt, usize len, va_list* vargs);
void gfmt_print(void* ctx, const char* fmt, ...);
