#include "stdio.h"

#include <stdarg.h>
#include <stdbool.h>
#include "base.h"

#include "gprint.h"

#include <arch/x86.h>
#include <io/console.h>

static char putc_buffer[32];
static u32 putc_buffer_pos = 0;

void clrscr() { con_clear(); }

void flush_terminal() {
    con_write(putc_buffer, putc_buffer_pos);
    putc_buffer_pos = 0;
}

void putc(char ch) {
    if(putc_buffer_pos == sizeof(putc_buffer)) {
        con_write(putc_buffer, 32);
        putc_buffer_pos = 0;
    }

    putc_buffer[putc_buffer_pos] = ch;
    putc_buffer_pos++;
}

void puts(const char* str) {
    flush_terminal();
    con_write(str, strlen(str));
}

void gp_putc(char ch) {
    putc(ch);
}

void vprintf(const char* fmt, va_list vargs) {
    gp_printer(fmt, strlen(fmt), &vargs);
}

void printf(const char* fmt, ...) {
    va_list vargs;
    va_start(vargs, fmt);
    gp_printer(fmt, strlen(fmt), &vargs);
    va_end(vargs);
}
