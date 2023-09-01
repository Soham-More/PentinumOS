[bits 32]

; void x86_raise(uint32_t error_code);
global x86_raise
x86_raise:
    mov eax, [esp + 4]
    mov ebx, [esp]
    int 81h
    ret
