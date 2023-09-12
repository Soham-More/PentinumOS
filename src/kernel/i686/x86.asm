
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

; uint32_t _cdecl x86_flushTLB();
global x86_flushTLB
x86_flushTLB:
    [bits 32]

    mov eax, cr3
    mov cr3, eax

    ret

; _import uint32_t _asmcall x86_flushCache();
global x86_flushCache
x86_flushCache:
    [bits 32]
    invd
    ret
