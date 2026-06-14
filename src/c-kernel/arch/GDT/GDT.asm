[ bits 32 ]
; i686_load_gdt(GDT_DESC* gdt_desc, u16 code_segment, u16 data_segment);

global i686_load_gdt
i686_load_gdt:
    ; init call frame
    push ebp
    mov ebp, esp

    ; load gdt
    mov eax, [ebp + 8]
    lgdt [eax]

    ; reload code segment
    mov eax, [ebp + 12]
    push eax
    ; far return to new Code Segment
    push .reload_cs
    retf

.reload_cs:
    ; reload data segments
    mov ax, [ebp + 16]
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; restore old call frame
    mov esp, ebp
    pop ebp
    ret
