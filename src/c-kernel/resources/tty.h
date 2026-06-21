// tty is a terminal interface for the kernel
// providing a thread safe interface for printing formatted strings to the console
// and providing buffered output to the console
#pragma once
#include <includes.h>
#include <stdarg.h>

#include "console.h"
#include <syscore/threads.h>

#define TTY_NO_BUFFERING 0x1
#define TTY_FLUSH_ON_NEWLINE 0x2
#define TTY_USE_LOCKS 0x4

typedef struct tty_t {
    char name[16];

    console_t* console;
    thread_mutex_t mutex;

    char* buffer;
    usize buffer_size;
    usize buffer_pos;
    
    u32 flags;

    struct tty_t* next;
} tty_t;

tty_t* construct_tty(const char* name, console_t* console, usize buffer_size, u32 flags);
tty_t* get_tty(const char* name);

void tty_clear(tty_t* tty);
void tty_flush(tty_t* tty);

// thread safe printf to the tty
void tty_vprintf(u32 color, tty_t* tty, const char* fmt, va_list vargs);
// thread safe printf to the tty
void tty_printf(u32 color, tty_t* tty, const char* fmt, ...);

// lock and unlock the tty
// when you need guarenteed exclusive access to the tty
tty_t* tty_lock(tty_t* tty);
void tty_unlock(tty_t** tty);
#define TTY_AQUIRE_SCOPED_LOCK(tty) tty_t* __tty_lock __attribute__((cleanup(tty_unlock))) = tty_lock((tty));