#pragma once

#define _cdecl /* extern "C" */
#define _packed __attribute__((packed))

#ifdef _novscode
#define _export extern
#define _import extern
#define _asmcall __attribute__((cdecl))
#define _no_stack_trace __attribute__((no_instrument_function))
#endif

#ifndef _novscode
#define _export /* extern "C" */
#define _import /* extern "C" */
#define _asmcall /* __attribute__((cdecl)) */
#define _no_stack_trace /* __attribute__((no_instrument_function)) */
#endif


