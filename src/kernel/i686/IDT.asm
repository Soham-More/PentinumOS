
[ bits 32 ]

; i686_load_idt(void* IDT_DESC)

global i686_load_idt
i686_load_idt:
    ; init call frame
    push ebp
    mov ebp, esp

    ; load idtr
    mov eax, [ebp + 8]
    lidt [eax]

    ; previous call frame
    mov esp, ebp
    pop ebp

    ret
