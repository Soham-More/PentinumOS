#pragma once

#include <includes.h>
#include <utils/heap.h>

#define CON_COLOR_BLACK   (0x0)
#define CON_COLOR_RED     (0x4)
#define CON_COLOR_BLUE    (0x1)
#define CON_COLOR_GREEN   (0x2)
#define CON_COLOR_WHITE   (0x7)
#define CON_COLOR_CYAN    (0x3)
#define CON_COLOR_MAGENTA (0x5)
#define CON_COLOR_BROWN   (0x6)

typedef struct console_t {
    char name[16];
    void (*write)(struct console_t *, const char*, usize);
    i32  (*read) (struct console_t *, char*, usize);
    void (*clear)(struct console_t *);
    u32  text_color;
    void* data;
    uid_t uid;
    // ptr to next console
    struct console_t* next;
} console_t;

void con_ignore_write(console_t *, const char*, usize);
i32  con_ignore_read (console_t *, char*, usize);
void con_ignore_clear(console_t *);

console_t construct_console(const char name[16]);
err_t register_console(console_t con);

console_t* con_find(const char name[16]);
console_t* con_get_safe();
err_t con_set_safe(console_t* con);
