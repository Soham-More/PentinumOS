#pragma once

#include <stdint.h>
#include <stdbool.h>

#define _cdecl __attribute__((cdecl))
#define _packed __attribute__((packed))
#define null (char)0
#define nullptr (void*)0

#define _debug_log_uint(x) printf("debug value of %s: %u\n", #x, x)
#define _debug_log_uint_h(x) printf("debug value of %s: 0x%x\n", #x, x)

#define SECTOR_SIZE         (unsigned long)512

#ifdef r_Debug

#define dcall(x) x

#endif
#ifndef r_Debug

#define dcall(x)

#endif
