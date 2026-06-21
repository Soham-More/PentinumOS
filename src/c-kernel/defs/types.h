#pragma once

#include <stdint.h>

#define null (char)0
#define nullptr 0

#define MAX_STACK_DEPTH 4096

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

typedef u32 usize;
typedef u32 ptr_t;
typedef i32 uid_t;
typedef u32 pid_t;
typedef i32 err_t;

// invalid values
#define invalid_u32 UINT32_MAX
#define invalid_u16 UINT16_MAX
#define invalid_u8  UINT8_MAX

#define invalid_uid invalid_u32

// fptrs
// page allocator, takes the number of pages to allocate
typedef void*(*page_allocator_t)(usize);

