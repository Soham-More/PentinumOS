ENTRY(start)
OUTPUT_FORMAT("elf32-i386")
phys = 0x00600000;
page_allocator_size = 0x10000;  /* 64KiB page allocator */
heap_size = 0x100000;  /* 1MB heap */
stack_size = 0x1000; /* 4 KiB stack(1 page) */

SECTIONS
{
    . = phys;

    .idt                : { __idt_start = .;        *(.idt)     }
    .text               : { __text_start = .;       *(.text)    }
    .data               : { __data_start = .;       *(.data)    }
    .rodata             : { __rodata_start = .;     *(.rodata)  }
    .bss                : { __bss_start = .;        *(.bss)     }
    
    . = ALIGN(0x10);
    .page_allocator         : { 
        __page_allocator_start = .; 
        . += page_allocator_size;
        __page_allocator_end = .;
    }
    . = ALIGN(0x10);
    .stack         : { 
        __stack_start = .; 
        . += stack_size;
        __stack_end = .;
    }
    . = ALIGN(0x1000);
    .heap         : {
        __heap_start = .;
        . += heap_size;
        __heap_end = .;
    }

    __end = .;
}
