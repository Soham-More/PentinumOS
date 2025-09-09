#pragma once

#define STD_OUT_OF_RANGE_INDEX 0x0
#define STD_UNWRAP_FAIL        0x1

namespace std
{
    inline static const char* errors_desc[] = {
        "[PANIC][std] Out of range index access",
        "[PANIC][std] unwrap of optional_reference failed!",
    };
}


