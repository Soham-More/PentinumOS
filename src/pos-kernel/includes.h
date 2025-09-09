#pragma once

#include <stdint.h>
#include <stdbool.h>

#define _cdecl __attribute__((cdecl))
#define _packed __attribute__((packed))

#define null (char)0
#define nullptr 0

#define MAX_STACK_DEPTH 4096

#define invalid_u32 ((uint32_t)-1)
#define invalid_u16 UINT16_MAX

typedef uint32_t size_t;
typedef uint32_t ptr_t;

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

typedef u32 size_t;
typedef u32 ptr_t;
typedef i32 uid_t;
typedef u32 pid_t;

// the first page is invalid
#define IS_ERR_PTR(x) (reinterpret_cast<u32>(x) < 0x1000)
#define ERR_PTR(x, type) reinterpret_cast<type*>(x)
#define ERR_CAST(x) reinterpret_cast<u32>(x)

#define ENONODE  ((u32)0)
#define EINVPATH ((u32)1)
#define EINVOP   ((u32)2)
#define EINVARG  ((u32)3)
#define OP_SUCCESS ((u32)0x1000)

#define ptr_cast reinterpret_cast<ptr_t>

// Defines for UIDs
#define UID_IS_DRIVER_HINT(x) (x < 0)
#define UID_CLASS_USBCTRL ((uid_t)-2)
#define UID_CLASS_USB ((uid_t)-3)

#ifdef _novscode
#define _export extern "C"
#define _import extern "C"
#define _asmcall __attribute__((cdecl))
#define _no_stack_trace __attribute__((no_instrument_function))
#endif

#ifndef _novscode
#define _export /* extern "C" */
#define _import /* extern "C" */
#define _asmcall /* __attribute__((cdecl)) */
#define _no_stack_trace /* __attribute__((no_instrument_function)) */
#endif
