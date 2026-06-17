#pragma once

#include <includes.h>

typedef struct cpu_info_t {
    char vendor[13]; // 12 chars + null terminator
    u8 family;
    u8 model;
} cpu_info_t;

cpu_info_t cpu_get_info();
void cpu_panic(const char* filename, const char* function, usize lineno, err_t error, const char* fmt, ...);

#define panic(error, ...) cpu_panic(__FILE__, __func__, __LINE__, error, __VA_ARGS__)
#define panic_on_err(call, ...) \
    { \
        err_t _err = (call); \
        if(_err != ESUCCESS) \
        { \
            cpu_panic(__FILE__, __func__, __LINE__, _err, __VA_ARGS__); \
        } \
    }
#define panic_on_err_ptr(ptr, ...) \
    { \
        if(IS_ERR_PTR((ptr))) \
        { \
            cpu_panic(__FILE__, __func__, __LINE__, ERR_CAST((ptr)), __VA_ARGS__); \
        } \
    }
#define panic_if(cond, error, ...) \
    { \
        if((cond)) \
        { \
            cpu_panic(__FILE__, __func__, __LINE__, (error), __VA_ARGS__); \
        } \
    }
