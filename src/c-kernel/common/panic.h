#pragma once

// panic error codes
#define PANIC_OK ((u32)0x00)

#define PANIC_CLASS_GENERIC      ((u32)0x00)
#define PANIC_UNEXPECTED_FAILURE (PANIC_CLASS_GENERIC | 0x01)

#define PANIC_CLASS_MEMORY       ((u32)0x10)
#define PANIC_BAD_MEMORY_REQUEST (PANIC_CLASS_MEMORY | 0x01)
