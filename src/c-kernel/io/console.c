#include "console.h"

#include <c-utils/c-utils.h>
#include <memory/memory.h>

void con_ignore_write(console_t *, const char *, usize){}
i32  con_ignore_read (console_t *, char*, usize){return ESUCCESS;}
void con_ignore_clear(console_t *){}

struct {
    console_t earlyconsole;
    console_t* default_console;

    usize link_size;
    console_t* link_head;
    console_t* link_tail;
} g_con_data;

void initialize_console_manager(console_t earlyconsole) {
    g_con_data.earlyconsole = earlyconsole;
    g_con_data.earlyconsole.uid = 0;
    g_con_data.earlyconsole.next = nullptr;

    g_con_data.link_head = &g_con_data.earlyconsole;
    g_con_data.link_tail = &g_con_data.earlyconsole;
    g_con_data.default_console = &g_con_data.earlyconsole;
}
console_t construct_console(const char name[16]) {
    console_t console;
    memcpy(console.name, name, 16);
    console.text_color = CON_COLOR_WHITE;
    console.data = nullptr;
    console.uid = invalid_u32;
    
    console.write = con_ignore_write;
    console.read  = con_ignore_read;
    console.clear = con_ignore_clear;

    return console;
}

err_t register_console(heap_allocator_t* allocator, console_t con) {
    console_t* new_console = COPY_TO_HEAP(allocator, con);
    if(IS_INV_PTR(new_console)) return ERR_CAST(new_console);

    new_console->next = nullptr;
    new_console->uid = g_con_data.link_size;
    
    g_con_data.link_size++;
    g_con_data.link_tail->next = new_console;
    g_con_data.link_tail = new_console;

    return ESUCCESS;
}
bool con_set_default(uid_t con_uid) {
    if(con_uid >= g_con_data.link_size) return false;

    console_t* ptr = g_con_data.link_head;
    for(usize i = 0; i < con_uid; i++)
    {
        ptr = ptr->next;
    }
    g_con_data.default_console = ptr;

    return true;
}

console_t* con_get_default()
{
    return g_con_data.default_console;
}
console_t* con_find(const char name[16]) {
    console_t* curr_con = g_con_data.link_head;
    while(curr_con) {
        // if equal, then return
        if(strcmp(curr_con->name, name, 16)) {
            return curr_con;
        }
        curr_con = curr_con->next;
    }
    return ERR_PTR(console_t, ENOTFOUND);
}

void con_write(const char* buffer, usize len) {
    return g_con_data.default_console->write(g_con_data.default_console, buffer, len);
}
i32  con_read(char* buffer, usize len) {
    return g_con_data.default_console->read(g_con_data.default_console, buffer, len);
}
void con_clear() {
    return g_con_data.default_console->clear(g_con_data.default_console);
}
