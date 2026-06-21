#pragma once

// pointer math
#define PTR_DIFF_I32(x, y) (i32)(x - y)

// bit twiddling
#define SET_MASKED_FLAGS(mask, value, set_flags) ((value) & (~mask)) | ((set_flags) & (mask))

// debug behaviour
#define VALIDATE(x, ret) \
    if(!(x)) { \
        return ret; \
    }


