#pragma once

#include <includes.h>
#include "TSS.h"

/*

How GDT's and segmentation work:

    each entry of GDT describes an 'segment'
    A segment simply a subset of the memory.
    It is used to safely manage memory.(Just like paging)

    The size, position, type and access flags are given by the entries in GDT

    the segment registers point to entries in GDT( they store offset to these entries )

    Since the kernel mode code segment is at offset 0x08, i686_GDT_CODE_SEGMENT is 0x08
    similarly DATA_SEGMENT is 0x10

*/

#define i686_GDT_CODE_SEGMENT 0x08
#define i686_GDT_DATA_SEGMENT 0x10

void i686_init_gdt();

void x86_gdt_add_tss(tss_entry_t* tss, u32 gdt_entry_id);

