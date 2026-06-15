; _import void kmt_switch_task(ktcb_t* current_thread, ktcb_t* next_thread);
global kmt_switch_task
kmt_switch_task:

    ;Save previous task's state

    ;Notes:
    ;  For cdecl; EAX, ECX, and EDX are already saved by the caller and don't need to be saved again
    ;  EIP is already saved on the stack by the caller's "CALL" instruction
    ;  The task isn't able to change CR3 so it doesn't need to be saved
    ;  Segment registers are constants (while running kernel code) so they don't need to be saved

    push ebx
    push esi
    push edi
    push ebp

    mov edi,[ebp + 8]             ;edi = address of the previous task's "thread control block"
    mov [edi + 0],esp             ;Save ESP for previous task's kernel stack in the thread's TCB

    ;Load next task's state

    mov esi,[ebp + 12]            ;esi = address of the next task's "thread control block" (parameter passed on stack)
    mov edx,esi                   ;Current task's TCB is the next task TCB

    mov esp,[esi + 0]             ;Load ESP for next task's kernel stack from the thread's TCB
    mov eax,[esi + 8]             ;eax = address of page directory for next task
    mov ebx,[esi + 4]             ;ebx = address for the top of the next task's kernel stack
    mov ecx, [ebp + 12]           ;ecx = the TSS struct
    mov [ecx + 4],ebx             ;Adjust the ESP0 field in the TSS (used by CPU for for CPL=3 -> CPL=0 privilege level changes)
    mov ecx,cr3                   ;ecx = previous task's virtual address space

    cmp eax,ecx                   ;Does the virtual address space need to be changed?
    je .doneVAS                   ; no, virtual address space is the same, so don't reload it and cause TLB flushes
    mov cr3,eax                   ; yes, load the next task's virtual address space
.doneVAS:

    pop ebp
    pop edi
    pop esi
    pop ebx

    ret                           ;Load next task's EIP from its kernel stack