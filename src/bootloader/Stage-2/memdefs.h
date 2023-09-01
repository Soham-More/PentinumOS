#pragma once

// 0x00000000 - 0x000003FF - interrupt vector table
// 0x00000400 - 0x000004FF - BIOS data area

#define MEMORY_MIN          0x00000500
#define MEMORY_MAX          0x00080000

// 0x00000500 - 0x00010500 - FAT driver
#define MEMORY_FAT_ADDR     ((void*)0x20000)
#define MEMORY_FAT_SIZE     0x00010000

#define MEMORY_ELF_ADDR     ((void*)0x30000)

#define PAGE_SIZE 4096

#define PAGE_DIRECTORY_PHYSICAL_ADDRESS     (void*)0x100000
#define PAGE_TABLE_ARRAY_PHYSICAL_ADDRESS   (void*)(0x100000 + PAGE_SIZE)

#define KERNEL_LOAD_ADDR    ((void*)0x40000)
#define KERNEL_ADDR         ((void*)0x600000)

#define MEMORY_ADDR(x)      MEMORY_FAT_ADDR + x * SECTOR_SIZE

// 0x00020000 - 0x00030000 - stage2

// 0x00030000 - 0x00080000 - free

// 0x00080000 - 0x0009FFFF - Extended BIOS data area
// 0x000A0000 - 0x000C7FFF - Video
// 0x000C8000 - 0x000FFFFF - BIOS
