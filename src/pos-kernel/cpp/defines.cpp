#include "defines.hpp"

#include <io/io.h>

extern "C" void __cxa_pure_virtual()
{
	log_error("[C++] Error: called pure virtual function\n");
}
extern "C" int __cxa_atexit(void (*destructor) (void *), void *arg, void *dso)
{
    arg;
    dso;
    return 0;
}
extern "C" void __cxa_finalize(void *f)
{
    f;
}
