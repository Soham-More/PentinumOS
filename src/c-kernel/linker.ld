ENTRY(start)
OUTPUT_FORMAT("elf32-i386")
stack = 0x00500000;
phys  = 0x00600000;
stack_size    = 0x4000; /* 16 KiB main (intr) stack */
heap_size     = 0x200000;  /* 1MB heap */
dbgstack_size = 0x1000; /* 4 KiB stack(1 page) */

SECTIONS
{
    . = stack;
    __stack_rsvd_start = .;

    /* stack also used by the init thread */
    __idle_thread_intr_stack_start = .;
    . += stack_size;
    __idle_thread_intr_stack_end = .;

    __idle_thread_exec_stack_start = .;
    . += stack_size;
    __idle_thread_exec_stack_end = .;

    __syscore_thread_intr_stack_start = .;
    . += stack_size;
    __syscore_thread_intr_stack_end = .;

    __syscore_thread_exec_stack_start = .;
    . += stack_size;
    __syscore_thread_exec_stack_end = .;

    __stack_rsvd_end = .;

    . = phys;

    .idt                : { __idt_start = .;        *(.idt)     }
    .text               : { __text_start = .;       *(.text)    }
    .data               : { __data_start = .;       *(.data)    }
    .rodata             : { __rodata_start = .;     *(.rodata)  }
    .bss                : { __bss_start = .;        *(.bss)     }
    
    . = ALIGN(0x1000);
    __heap_rsvd_start = .;

    . = ALIGN(0x1000);
    __root_heap_start = .;
    . += heap_size;
    __root_heap_end = .;
    
    . = ALIGN(0x1000);
    __idle_heap_start = .;
    . += heap_size;
    __idle_heap_end = .;
    
    . = ALIGN(0x1000);
    __buddy_heap_start = .;
    . += heap_size;
    __buddy_heap_end = .;
    
    . = ALIGN(0x1000);
    __kmt_heap_start = .;
    . += heap_size;
    __kmt_heap_end = .;

    . = ALIGN(0x1000);
    __syscore_heap_start = .;
    . += heap_size;
    __syscore_heap_end = .;

    __heap_rsvd_end = .;


    __end = .;
}
