#include "dcom.h"

#include "../priv.h"
#include <io/console.h>

#include <arch/x86.h>

#define USE_QEMU_DCOM

void dcom_putc(char c)
{
#ifdef USE_QEMU_DCOM
    x86_outb(0xE9, c);
#endif
}

void dcom_write(console_t* console, const char* buffer, usize len)
{
    for(size_t i = 0; i < len; i++) dcom_putc(buffer[i]);
}

console_t dcom_get_earlyconsole() {
    console_t dcom_console = {
        .name = "dcom0",
        .write = dcom_write,
        .read = con_ignore_read,
        .clear = con_ignore_clear,
        .text_color = CON_COLOR_WHITE,
        .data = nullptr,
    };
    return dcom_console;
}


err_t initialize_dcom_drivers(){
    heap_allocator_t* alloca = get_driver_allocator();

    console_t dcom_console = {
        .name = "dcom0",
        .write = dcom_write,
        .read = con_ignore_read,
        .clear = con_ignore_clear,
        .text_color = CON_COLOR_WHITE,
        .data = nullptr,
    };

    // register a new console
    return register_console(alloca, dcom_console);
}

