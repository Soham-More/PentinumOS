#include "tty.h"
#include <panic/tty.h>

#include <stdarg.h>
#include <stdbool.h>
#include <utils/cstdlib.h>

#include <boot/init.h>
#include <arch/x86.h>

#include "impl/gprint.h"

static tty_t g_earlytty;
static tty_t* g_tty_head = nullptr;
static tty_t* g_tty_tail = nullptr;
static volatile bool g_panic = false;

tty_t* initialize_tty(const char* name, console_t* earlyconsole) {
    g_earlytty.console = earlyconsole;
    g_earlytty.buffer = nullptr;
    g_earlytty.buffer_size = 0;
    g_earlytty.buffer_pos = 0;
    g_earlytty.flags = TTY_NO_BUFFERING;
    memcpy(g_earlytty.name, name, sizeof(g_earlytty.name));

    g_tty_head = &g_earlytty;
    g_tty_tail = &g_earlytty;
    return &g_earlytty;
}
tty_t* construct_tty(const char* name, console_t* console, usize buffer_size, u32 flags) {
    if(IS_ERR_PTR(console)) return ERR_PTR(tty_t, EINVPTR);

    thread_mutex_t mutex = invalid_u16;
    if(flags & TTY_USE_LOCKS) {
        mutex = kmt_create_mutex();
        if(KMT_IS_INVALID_MUTEX(mutex)) return ERR_PTR(tty_t, KMT_GET_ERR_MUTEX(mutex));
    }
    GET_GLOBAL_ALLOCATOR(galloc);
    
    tty_t* tty = (tty_t*)malloc(galloc, sizeof(tty_t));
    if(IS_ERR_PTR(tty)) return tty;

    memcpy(tty->name, name, sizeof(tty->name));
    tty->console = console;
    tty->buffer = (char*)malloc(galloc, buffer_size);
    tty->buffer_size = buffer_size;
    tty->flags = flags;
    tty->mutex = mutex;

    g_tty_tail->next = tty;
    g_tty_tail = tty;
    
    return tty;
}
tty_t* get_tty(const char* name) {
    tty_t* tty = g_tty_head;
    while(tty != nullptr) {
        if(strcmp(tty->name, name, sizeof(tty->name))) return tty;
        tty = tty->next;
    }
    return nullptr;
}

// helpers
tty_t* tty_lock(tty_t* tty) {
    if((tty->flags & TTY_USE_LOCKS) == 0) return nullptr;
    if(kmt_is_mutex_owned(tty->mutex)) return nullptr;
    err_t err = kmt_lock_mutex(tty->mutex);
    if(err != ESUCCESS) {
        tty->console->write(tty->console, "Failed to acquire TTY lock... bypassing lock\n", 46);
        return tty;
    }
    return tty;
}
void tty_unlock(tty_t** tty) {
    if(*tty == nullptr) return;
    if(((*tty)->flags & TTY_USE_LOCKS) == 0) return;
    err_t err = kmt_unlock_mutex((*tty)->mutex);
    // if the mutex is not owned by the current thread
    // it means that the lock was not aquired successfully, so we can ignore this error
    if((err != ESUCCESS) && (err != ENOTOWNED)) {
        (*tty)->console->write((*tty)->console, "Failed to release TTY lock!\n", 29);
    }
}

void tty_clear(tty_t* tty) {
    TTY_AQUIRE_SCOPED_LOCK(tty);
    tty->console->clear(tty->console);
}
void tty_flush(tty_t* tty) {
    if(tty->flags & TTY_NO_BUFFERING) return;
    TTY_AQUIRE_SCOPED_LOCK(tty);
    tty->console->write(tty->console, tty->buffer, tty->buffer_pos);
    tty->buffer_pos = 0;
}
// assumes the caller has aquired the lock
void gp_putc(void* ctx, char ch) {
    if(g_panic) {
        // if we are in a panic state, we want to bypass the tty buffer and write directly to the console
        // we also want to write as safe as possible
        console_t* con = con_get_safe();
        con->write(con, &ch, 1);
        return;
    }

    tty_t *tty = (tty_t*)ctx;

    if(tty->flags & TTY_NO_BUFFERING) {
        tty->console->write(tty->console, &ch, 1);
        return;
    }
    if(tty->buffer_pos == tty->buffer_size) {
        tty->console->write(tty->console, tty->buffer, sizeof(tty->buffer));
        tty->buffer_pos = 0;
    }

    tty->buffer[tty->buffer_pos] = ch;
    tty->buffer_pos++;

    if(ch == '\n' && (tty->flags & TTY_FLUSH_ON_NEWLINE)) tty_flush(tty);
}

void tty_vprintf(u32 color, tty_t* tty, const char* fmt, va_list vargs) {
    g_panic = false;
    TTY_AQUIRE_SCOPED_LOCK(tty);
    u32 old_color = tty->console->text_color;
    // flush the buffer if the color is changing, so that the color change takes effect immediately
    if(old_color != color) tty_flush(tty);
    tty->console->text_color = color;
    gp_printer(tty, fmt, strlen(fmt), &vargs);
}
void tty_printf(u32 color, tty_t* tty, const char* fmt, ...) {
    g_panic = false;
    TTY_AQUIRE_SCOPED_LOCK(tty);
    u32 old_color = tty->console->text_color;
    // flush the buffer if the color is changing, so that the color change takes effect immediately
    if(old_color != color) tty_flush(tty);
    tty->console->text_color = color;
    va_list vargs;
    va_start(vargs, fmt);
    gp_printer(tty, fmt, strlen(fmt), &vargs);
    va_end(vargs);
}

void tty_panic_vprintf(const char* fmt, va_list vargs) {
    g_panic = true;
    gp_printer(nullptr, fmt, strlen(fmt), &vargs);
}
void tty_panic_printf(const char* fmt, ...) {
    g_panic = true;
    va_list vargs;
    va_start(vargs, fmt);
    gp_printer(nullptr, fmt, strlen(fmt), &vargs);
    va_end(vargs);
}
