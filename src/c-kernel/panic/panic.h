// only for use by the syscore and boot threads
#pragma once

#include <includes.h>

void kernel_panic(const char* filename, const char* function, usize lineno, err_t error, const char* fmt, ...);

#define kpanic(error, ...) kernel_panic(__FILE__, __func__, __LINE__, error, __VA_ARGS__)
#define kpanic_on_err(call, ...) \
    { \
        err_t _err = (call); \
        if(_err != ESUCCESS) \
        { \
            kernel_panic(__FILE__, __func__, __LINE__, _err, __VA_ARGS__); \
        } \
    }
#define kpanic_on_err_ptr(ptr, ...) \
    { \
        if(IS_ERR_PTR((ptr))) \
        { \
            kernel_panic(__FILE__, __func__, __LINE__, ERR_CAST((ptr)), __VA_ARGS__); \
        } \
    }
#define kpanic_if(cond, error, ...) \
    { \
        if((cond)) \
        { \
            kernel_panic(__FILE__, __func__, __LINE__, (error), __VA_ARGS__); \
        } \
    }
