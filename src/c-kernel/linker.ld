ENTRY(start)
OUTPUT_FORMAT("elf32-i386")
stack = 0x00500000;
phys  = 0x00600000;
stack_size    = 0x4000; /* 16 KiB main stack */
heap_size     = 0x200000;  /* 1MB heap */
dbgstack_size = 0x1000; /* 4 KiB stack(1 page) */

SECTIONS
{
    . = stack;
    .kernel_stack : {
        __kernel_stack_begin = .;
        . += stack_size;
        __kernel_stack_end = .;
    }

    . = phys;

    .idt                : { __idt_start = .;        *(.idt)     }
    .text               : { __text_start = .;       *(.text)    }
    .data               : { __data_start = .;       *(.data)    }
    .rodata             : { __rodata_start = .;     *(.rodata)  }
    .bss                : { __bss_start = .;        *(.bss)     }
    
    . = ALIGN(0x10);
    .dbgstack         : { 
        __dbgstack_start = .;
        . += dbgstack_size;
        __dbgstack_end = .;
    }
    . = ALIGN(0x1000);
    .global_heap   : {
        __global_heap_start = .;
        . += heap_size;
        __global_heap_end = .;
    }
    . = ALIGN(0x1000);
    .buddy_heap    : {
        __buddy_heap_start = .;
        . += heap_size;
        __buddy_heap_end = .;
    }

    __end = .;
}
