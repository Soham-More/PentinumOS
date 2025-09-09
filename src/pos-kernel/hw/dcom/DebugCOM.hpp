#pragma once

#include <includes.h>
#include <arch/x86.h>

#include <io/console.hpp>

namespace DCOM
{
    void putc(char c);

    con::console get_console();
}
