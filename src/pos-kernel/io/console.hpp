#pragma once

#include <includes.h>

#define CON_COLOR_BLACK (0x0)
#define CON_COLOR_RED   (0x4)
#define CON_COLOR_BLUE  (0x1)
#define CON_COLOR_GREEN (0x2)
#define CON_COLOR_WHITE (0x7)

namespace con
{
    struct console {
        char name[16];
        void (*write)(console *, const char*, size_t);
        i32  (*read) (console *, char*, size_t);
        void (*clear)(console *);
        u32  text_color;
        void* data;
        uid_t uid;
        // ptr to next console
        console* next;
    };

    void ignore_write(console *, const char*, size_t);
    i32  ignore_read (console *, char*, size_t);
    void ignore_clear(console *);

    void init(console earlyconsole);

    void register_console(console con);
    bool set_default(uid_t con_uid);
    console* get_default();

    void write(const char* buffer, size_t len);
    i32  read (char* buffer, size_t len);
    void clear();
}

