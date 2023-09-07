#pragma once

#include <stdint.h>
#include <stdbool.h>

#define _cdecl __attribute__((cdecl))
#define _packed __attribute__((packed))
#define null (char)0
#define nullptr 0

typedef uint32_t size_t;

#ifdef _novscode
#define _export extern "C"
#define _import extern "C"
#define _asmcall __attribute__((cdecl))
#endif

#ifndef _novscode
#define _export /* extern "C" */
#define _import /* extern "C" */
#define _asmcall /* __attribute__((cdecl)) */
#endif
