#include "fmt.h"

#include "impl/gfmt.h"

typedef struct fmt_ctx_t {
    char* buffer;
    usize buffer_size;
    usize buffer_index;
    heap_allocator_t* heap;
    err_t error;
} fmt_ctx_t;

void gfmt_putc(void* ctx, char ch) {
    fmt_ctx_t* fmt_ctx = (fmt_ctx_t*)ctx;
    if(fmt_ctx->error != EPENDING) return;

    if(fmt_ctx->buffer_index < fmt_ctx->buffer_size) {
        fmt_ctx->buffer[fmt_ctx->buffer_index] = ch;
        fmt_ctx->buffer_index++;
    } else if(fmt_ctx->heap) {
        fmt_ctx->buffer = realloc(fmt_ctx->heap, fmt_ctx->buffer, 2 * fmt_ctx->buffer_size);
        fmt_ctx->buffer_size *= 2;
        fmt_ctx->buffer[fmt_ctx->buffer_index] = ch;
        fmt_ctx->buffer_index++;
    } else {
        fmt_ctx->error = ESTRTOOBIG;
    }
}

// format to buffer, returns 0 on failure
usize format(char* buffer, usize buffer_size, const char* fmt, ...) {
    fmt_ctx_t ctx = {
        .buffer = buffer,
        .buffer_size = buffer_size,
        .buffer_index = 0,
        .heap = nullptr,
        .error = EPENDING,
    };

    va_list vargs;
    va_start(vargs, fmt);
    gfmt_printer(&ctx, fmt, gfmt_cstr_len(fmt), &vargs);
    va_end(vargs);

    if(ctx.error != EPENDING) return 0;

    // null terminate the string
    if(ctx.buffer_index < ctx.buffer_size) {
        ctx.buffer[ctx.buffer_index] = '\0';
    } else if(ctx.heap) {
        ctx.buffer = realloc(ctx.heap, ctx.buffer, 2 * ctx.buffer_size);
        ctx.buffer_size *= 2;
        ctx.buffer[ctx.buffer_index] = '\0';
    } else {
        return 0;
    }

    return ctx.buffer_index;
}
// format to heap allocated buffer, returns err ptr on failure
char* formatA(heap_allocator_t* heap, const char* fmt, ...) {
    fmt_ctx_t ctx = {
        .buffer = nullptr,
        .buffer_size = 0,
        .buffer_index = 0,
        .heap = heap,
        .error = EPENDING,
    };

    va_list vargs;
    va_start(vargs, fmt);
    gfmt_printer(&ctx, fmt, gfmt_cstr_len(fmt), &vargs);
    va_end(vargs);

    if(ctx.error != EPENDING) return ERR_PTR(char, ctx.error);

    // null terminate the string
    if(ctx.buffer_index < ctx.buffer_size) {
        ctx.buffer[ctx.buffer_index] = '\0';
    } else {
        ctx.buffer = realloc(ctx.heap, ctx.buffer, 2 * ctx.buffer_size);
        ctx.buffer_size *= 2;
        ctx.buffer[ctx.buffer_index] = '\0';
    }

    return ctx.buffer;
}

