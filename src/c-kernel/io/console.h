#pragma once

#include <includes.h>
#include <memory/memory.h>

#define CON_COLOR_BLACK (0x0)
#define CON_COLOR_RED   (0x4)
#define CON_COLOR_BLUE  (0x1)
#define CON_COLOR_GREEN (0x2)
#define CON_COLOR_WHITE (0x7)

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

void initialize_console_manager(console_t earlyconsole);

console_t construct_console(const char name[16]);
err_t register_console(heap_allocator_t* allocator, console_t con);
bool con_set_default(uid_t con_uid);

console_t* con_get_default();
console_t* con_find(const char name[16]);

void con_write(const char* buffer, usize len);
i32  con_read (char* buffer, usize len);
void con_clear();

