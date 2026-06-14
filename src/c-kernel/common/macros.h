#pragma once

// pointer math
#define PTR_DIFF_I32(x, y) (i32)(x - y)

// bit twiddling
#define MIX_MASKED(target, flags, mask) ((target) & (~mask)) | ((flags) & (mask))

// debug behaviour
#define VALIDATE(x, ret) \
    if(!(x)) { \
        return ret; \
    }


