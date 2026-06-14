#pragma once

#include <includes.h>

#include <arch/i686.h>
#include <arch/prim_x86.hpp>

namespace ksch
{
    enum class thread_state_t : u8
    {
        IDLE,
        RUNNING,
        BLOCKED,
        TERMINATED
    };
    
    void update_tick();
}

