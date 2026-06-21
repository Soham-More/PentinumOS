#pragma once

// ERROR Checkers
#define IS_ERR_PTR(x) (((err_t)x) < 0x1000)
#define IS_INV_PTR(x) (((err_t)x) < 0x1000)
#define ERR_PTR(type, x) ((type*)(x))
#define ERR_CAST(x) ((err_t)x)
#define IS_ERROR(x) (((err_t)x) != ESUCCESS)

// errors
#define ESUCCESS    ERR_CAST(0x0)   // No error
#define EDEFAULT    ERR_CAST(0x1)   // default behaviour?

#define EMEMORY     ERR_CAST(0x10)   // Memory related errors
#define ENOMEM      (EMEMORY | 0x0)  // No memory left in current context
#define ENOPAGE     (EMEMORY | 0x1)  // requested page does not exist(not mapped to a physical addr)
#define EPAGEINUSE  (EMEMORY | 0x2)  // requested page is in use
#define EPOOLFULL   (EMEMORY | 0x3)  // a internal memory pool is full
#define ESTRTOOBIG  (EMEMORY | 0x4)  // a requested string is too big to fit in the provided buffer

#define EINV        ERR_CAST(0x20)   // Invalid operation/context
#define EINVPT      (EINV | 0x0)     // Invalid pagetable(happens if an allocator is called in wrong pagetable)
#define EINVSELF    (EINV | 0x1)     // Invalid self(happens if invalid data structure is passed)
#define EINVSTATE   (EINV | 0x2)     // Invalid state(happens if a data structure is in invalid state)
#define EINVAL      (EINV | 0x3)     // Invalid argument(happens if an invalid argument is passed to a function)
#define EINVPTR     (EINV | 0x4)     // Invalid pointer(happens if an invalid pointer is passed to a function)

#define EUNEXP      ERR_CAST(0x30)   // unexpected error
#define EUNEXPEXEC  (EUNEXP | 0x0)   // unexpected code execution(Ex. unreachable code is executed)
#define EUNEXPARG   (EUNEXP | 0x1)   // unexpected argument passed

#define ESWMT       ERR_CAST(0x40)   // software multithreading related errors
#define ETERMINATED (ESWMT | 0x0)    // attempting to wake up a thread that is already terminated
#define EINUSE      (ESWMT | 0x1)    // attempting to free a resource that is still in use
#define EUSEFREED   (ESWMT | 0x2)    // attempting to use a freed resource
#define ENOTOWNED   (ESWMT | 0x3)    // attempting to use a resource that is not owned by the current thread

#define EMISC       ERR_CAST(0x50)   // misc. error
#define ENOTFOUND   (EMISC | 0x0)    // a find operation could not find anything
#define EEXISTS     (EMISC | 0x1)    // a create operation found that the object already exists
#define EOUTOFRANGE (EMISC | 0x2)    // the given idx is out of range
#define ETIMEOUT    (EMISC | 0x3)    // operation timed out

#define ENOTERROR   ERR_CAST(0x70000000) // error code that does not indicate an error
#define EPENDING   (ENOTERROR | 0x0)    // no return (yet), pending operation

// error tracers, to find out where an error code is coming from
// for example, ERPC | EINVAL means that the error code is coming from an RPC function, and the error is due to an invalid argument
#define ERPC 0x100