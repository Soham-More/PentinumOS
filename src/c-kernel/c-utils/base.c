#include "base.h"

// cstring helpers
usize strlen(const char *a) {
    size_t size = 0;
    while(a[size] != 0) size++;
    return size;
}
bool strcmp(const char *a, const char *b, usize length) {
    for(usize i = 0; i < length; i++) {
        if(a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

void* memcpy(void* dst, const void* src, usize bytelength) {
    for(usize i = 0; i < bytelength; i++) {
        ((u8*)dst)[i] = ((u8*)src)[i];
    }
    return dst;
}

void  memset(u8* target, usize bytelength, u8 value) {
    for(usize i = 0; i < bytelength; i++) {
        target[i] = value;
    }
}

// math helpers
u64 div_ceil(u64 p, u64 q) { return (p + q - 1) / q; }
u64 div_floor(u64 p, u64 q) { return p / q; }
u32 ceil_to_2power(u32 v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

