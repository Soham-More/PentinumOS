#include "console.hpp"

#include <std/memory/heap.hpp>

namespace con
{
    void ignore_write(console *, const char *, size_t){}
    i32  ignore_read (console *, char*, size_t){return OP_SUCCESS;}
    void ignore_clear(console *){}

    console g_earlyconsole;
    console* default_console;
    
    size_t link_size;
    console* link_head;
    console* link_tail;

    void init(console earlyconsole)
    {
        g_earlyconsole = earlyconsole;
        g_earlyconsole.uid = 0;
        g_earlyconsole.next = nullptr;

        link_head = &g_earlyconsole;
        link_tail = &g_earlyconsole;
        default_console = &g_earlyconsole;
    }

    void register_console(console con)
    {
        console* new_console = new console(con);
        new_console->next = nullptr;
        new_console->uid = link_size;
        
        link_size++;
        link_tail->next = new_console;
        link_tail = new_console;
    }

    bool set_default(uid_t con_uid)
    {
        if(con_uid >= link_size) return false;

        console* ptr = link_head;
        for(size_t i = 0; i < con_uid; i++)
        {
            ptr = ptr->next;
        }
        default_console = ptr;

        return true;
    }

    console* get_default()
    {
        return default_console;
    }

    void write(const char* buffer, size_t len)
    {
        return default_console->write(default_console, buffer, len);
    }
    i32  read(char* buffer, size_t len)
    {
        return default_console->read(default_console, buffer, len);
    }
    void clear()
    {
        return default_console->clear(default_console);
    }
}
