#pragma once

#include <includes.h>

_import void _init();

extern "C" void __cxa_pure_virtual();
extern "C" int __cxa_atexit(void (*destructor) (void *), void *arg, void *dso);
extern "C" void __cxa_finalize(void *f);
