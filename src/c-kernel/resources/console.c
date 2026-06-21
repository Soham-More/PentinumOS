#include "console.h"

#include <boot/init.h>
#include <utils/heap.h>
#include <utils/cstdlib.h>

void con_ignore_write(console_t *, const char *, usize){}
i32  con_ignore_read (console_t *, char*, usize){return ESUCCESS;}
void con_ignore_clear(console_t *){}

struct {
    console_t earlyconsole;
    console_t* safe_console;

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
    g_con_data.safe_console = &g_con_data.earlyconsole;
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

err_t register_console(console_t con) {
    GET_GLOBAL_ALLOCATOR(galloc);
    console_t* new_console = COPY_TO_HEAP(galloc, con);
    if(IS_INV_PTR(new_console)) return ERR_CAST(new_console);

    new_console->next = nullptr;
    new_console->uid = g_con_data.link_size;
    
    g_con_data.link_size++;
    g_con_data.link_tail->next = new_console;
    g_con_data.link_tail = new_console;

    return ESUCCESS;
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

console_t* con_get_safe() {
    return g_con_data.safe_console;
}
err_t con_set_safe(console_t* con) {
    if(IS_ERR_PTR(con)) return EINVPTR;
    g_con_data.safe_console = con;
    return ESUCCESS;
}
