#pragma once

// ERROR Checkers
#define IS_ERR_PTR(x) (((err_t)x) < 4096)
#define IS_INV_PTR(x) (((err_t)x) < 4096)
#define ERR_PTR(type, x) (type*)(x)
#define ERR_CAST(x) ((err_t)x)
#define IS_ERROR(x) (((err_t)x) < 0)

// errors
#define ESUCCESS    ERR_CAST(0x0)   // No error
#define EDEFAULT    ERR_CAST(0x1)   // default behaviour?

#define EMEMORY     ERR_CAST(0x10)   // Memory related errors
#define ENOMEM      (EMEMORY | 0x0)  // No memory left in current context
#define ENOPAGE     (EMEMORY | 0x1)  // requested page does not exist(not mapped to a physical addr)
#define EPAGEINUSE  (EMEMORY | 0x2)  // requested page is in use

#define EINV        ERR_CAST(0x20)   // Invalid operation/context
#define EINVPT      (EINV | 0x0)     // Invalid pagetable(happens if an allocator is called in wrong pagetable)
#define EINVSELF    (EINV | 0x1)     // Invalid self(happens if invalid data structure is passed)
#define EINVSTATE   (EINV | 0x2)     // Invalid state(happens if a data structure is in invalid state)
#define EINVAL      (EINV | 0x3)     // Invalid argument(happens if an invalid argument is passed to a function)

#define EUNEXP      ERR_CAST(0x30)   // unexpected error
#define EUNEXPEXEC  (EUNEXP | 0x0)   // unexpected code execution(Ex. unreachable code is executed)
#define EUNEXPARG   (EUNEXP | 0x1)   // unexpected argument passed

#define EMISC       ERR_CAST(0x40)   // misc. error
#define ENOTFOUND   (EMISC | 0x0)    // a find operation could not find anything