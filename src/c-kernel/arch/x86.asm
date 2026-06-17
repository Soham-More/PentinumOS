
global x86_outb
x86_outb:
    [bits 32]
    mov dx, [esp + 4]
    mov al, [esp + 8]
    out dx, al
    ret

global x86_inb
x86_inb:
    [bits 32]
    mov dx, [esp + 4]
    xor eax, eax
    in al, dx
    ret

global x86_outw
x86_outw:
    [bits 32]
    mov dx, [esp + 4]
    mov ax, [esp + 8]
    out dx, ax
    ret

global x86_inw
x86_inw:
    [bits 32]
    mov dx, [esp + 4]
    xor eax, eax
    in ax, dx
    ret

global x86_outl
x86_outl:
    [bits 32]
    mov dx, [esp + 4]
    mov eax, [esp + 8]
    out dx, eax
    ret

global x86_inl
x86_inl:
    [bits 32]
    mov dx, [esp + 4]
    xor eax, eax
    in eax, dx
    ret

; void _cdecl x86_set_page_directory(void* page_directory_ptr);
global x86_set_page_directory
x86_set_page_directory:
    [bits 32]

    ; make new call frame
    push ebp             ; save old call frame
    mov ebp, esp          ; initialize new call frame

    mov eax, [ebp + 8]
    mov cr3, eax
    
    mov eax, cr0
    or eax, 0x80000001
    mov cr0, eax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret

; u32 _cdecl x86_get_cr0_register();
global x86_get_cr0_register
x86_get_cr0_register:
    [bits 32]
    mov eax, cr0
    ret

; u32 _cdecl x86_get_cr3_register();
global x86_get_cr3_register
x86_get_cr3_register:
    [bits 32]
    mov eax, cr3
    ret

; u32 _cdecl x86_flushTLB();
global x86_flushTLB
x86_flushTLB:
    [bits 32]

    mov eax, cr3
    mov cr3, eax

    ret

; _import u32 _asmcall x86_flushCache();
global x86_flushCache
x86_flushCache:
    [bits 32]
    invd
    ret

; _import void _asmcall x86_Panic();
global x86_Panic
x86_Panic:
    cli
    hlt

; _import void _asmcall x86_enable_interrupts();
global x86_enable_interrupts
x86_enable_interrupts:
    sti
    ret

; _import void _asmcall x86_disable_interrupts();
global x86_disable_interrupts
x86_disable_interrupts:
    cli
    ret

;_import uint32_t _asmcall x86_disable_intr_save();
global x86_disable_intr_save
x86_disable_intr_save:
    pushfd
    pop eax
    cli
    ret

;_import void _asmcall x86_restore_intr_saved(uint32_t eflags);
global x86_restore_intr_saved
x86_restore_intr_saved:
    push dword [esp + 4]
    popfd
    ret

; void x86_raise(u32 error_code);
global x86_raise
x86_raise:
    mov eax, [esp + 4]
    mov ebx, [esp]
    int 81h
    ret

