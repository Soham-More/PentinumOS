ENTRY(start)
OUTPUT_FORMAT("elf32-i386")
phys = 0x00600000;
heap_size     = 0x200000;  /* 1MB heap */
dbgstack_size = 0x1000; /* 4 KiB stack(1 page) */
early_obj_pool_size = 0x100; /* 256B memory to store some early objects */

SECTIONS
{
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
