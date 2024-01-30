#pragma once

#include <stdint.h>
#include <stdbool.h>

#define _cdecl __attribute__((cdecl))
#define _packed __attribute__((packed))
#define null (char)0
#define nullptr 0
#define MAX_STACK_DEPTH 4096

typedef uint32_t size_t;

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
