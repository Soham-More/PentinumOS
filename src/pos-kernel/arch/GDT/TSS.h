#pragma once

#include <includes.h>

struct tss_entry_t
{
    uint32_t prev_tss;   // Previous TSS (for task linking)
    uint32_t esp0;       // Stack pointer for privilege level 0 (kernel)
    uint32_t ss0;        // Stack segment for privilege level 0
    uint32_t esp1;       // Stack pointer for privilege level 1
    uint32_t ss1;        // Stack segment for privilege level 1
    uint32_t esp2;       // Stack pointer for privilege level 2
    uint32_t ss2;        // Stack segment for privilege level 2
    uint32_t cr3;        // Page directory base register
    uint32_t eip;        // Instruction pointer
    uint32_t eflags;     // Flags register
     // General purpose registers
    uint32_t eax;
    uint32_t ecx; 
    uint32_t edx;
    uint32_t ebx;
    uint32_t esp;
    uint32_t ebp;
    uint32_t esi;
    uint32_t edi; 
     // Segment registers
    uint32_t es;
    uint32_t cs;
    uint32_t ss;
    uint32_t ds;
    uint32_t fs;
    uint32_t gs;
    uint32_t ldtr;       // Local descriptor table selector
    uint16_t iopb;       // Trap on task switch
    uint16_t iomap_base; // I/O map base address
}_packed;


