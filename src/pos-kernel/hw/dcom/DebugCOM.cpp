#include "DebugCOM.hpp"

namespace DCOM
{
    void putc(char c)
    {
#ifdef USE_QEMU_DCOM
        x86_outb(0xE9, c);
#endif
    }

    void write(con::console* console, const char* buffer, size_t len)
    {
        for(size_t i = 0; i < len; i++) putc(buffer[i]);
    }

    con::console get_console()
    {
        return con::console{
            .name = "dcom0",
            .write = write,
            .read = con::ignore_read,
            .clear = con::ignore_clear,
            .text_color = 0,
        };
    }
}

