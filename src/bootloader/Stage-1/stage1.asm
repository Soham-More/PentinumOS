org 0x7C00
bits 16


%define ENDL 0x0D, 0x0A

start:
    ; setup data segments
    mov ax, 0           ; can't set ds/es directly
    mov ds, ax
    mov es, ax
    
    ; setup stack
    mov ss, ax
    mov sp, 0x7A00              ; stack grows downwards from where we are loaded in memory

    ; some BIOSes might start us at 07C0:0000 instead of 0000:7C00, make sure we are in the
    ; expected location
    push es
    push word .after
    retf

.after:
    ; show loading message
    mov si, msg_loading
    call puts

    ; read MBR to 0x7a00
    mov [DiskAddressPacket.lba], word 0
    mov [DiskAddressPacket.count], word 1
    mov [DiskAddressPacket.offset], word 0x7a00

    call disk_read

    ; jump to new MBR location
    jmp .loaded_MBR - 0x200

.loaded_MBR:

    mov si, msg_mbr_mov
    call puts

    ; find an active partition and load it's VBR
    ; current only loads Partition A's VBR

    ; loads partition a's first sector into 0x7c00
    ; and then executes it
    ; BEWARE: this code can only load 32 bit partitions
    mov eax, dword [PARTITION_A.lba]
    mov [DiskAddressPacket.lba], eax
    mov [DiskAddressPacket.count], word 1
    mov [DiskAddressPacket.offset], word VBR_LOAD_OFFSET

    ; push eax, VBR will get partition info by popping this value
    push eax

    call disk_read

    ; dl is loaded with drive no.
    ; eax has partition offset
    jmp VBR_LOAD_SEGMENT:VBR_LOAD_OFFSET

;
; Error handlers
;

read_error:
    mov si, msg_read_failed
    call puts
    jmp wait_key_and_reboot

kernel_not_found_error:
    mov si, msg_kernel_not_found
    call puts
    jmp wait_key_and_reboot

wait_key_and_reboot:
    mov ah, 0
    int 16h                     ; wait for keypress
    jmp 0FFFFh:0                ; jump to beginning of BIOS, should reboot

.halt:
    cli                         ; disable interrupts, this way CPU can't get out of "halt" state
    hlt

;
; Prints a string to the screen
; Params:
;   - ds:si points to string
;
puts:
    ; save registers we will modify
    push si
    push ax
    push bx

.loop:
    lodsb               ; loads next character in al
    or al, al           ; verify if next character is null?
    jz .done

    mov ah, 0x0E        ; call bios interrupt
    mov bh, 0           ; set page number to 0
    int 10h

    jmp .loop

.done:
    pop bx
    pop ax
    pop si    
    ret

;
; Disk routines
;

;
; Reads sectors from a disk
; Parameters:
;   - dl: drive number
; other params in DiskAddressPacket
;
disk_read:

    push ax                             ; save registers we will modify
    push bx
    push cx
    push dx
    push di
    push ds
    push si

    
    ; use extended disk reader
    mov ah, 42h
    
    ; set ds:si to DiskAddressPacket
    mov si, DiskAddressPacket

    mov di, 3                           ; retry count

.retry:
    pusha                               ; save all registers, we don't know what bios modifies
    stc                                 ; set carry flag, some BIOS'es don't set it
    int 13h                             ; carry flag cleared = success
    jnc .done                           ; jump if carry not set

    ; read failed
    popa
    call disk_reset

    dec di
    test di, di
    jnz .retry

.fail:
    ; all attempts are exhausted
    jmp read_error

.done:
    popa

    pop si
    pop ds
    pop di
    pop dx
    pop cx
    pop bx
    pop ax                             ; restore registers modified
    ret


;
; Resets disk controller
; Parameters:
;   dl: drive number
;
disk_reset:
    pusha
    mov ah, 0
    stc
    int 13h
    jc read_error
    popa
    ret


msg_loading:            db 'MBR has loaded', ENDL, 0
msg_mbr_mov:            db 'MBR has been moved', ENDL, 0
msg_read_failed:        db 'MBR: disk read failed, rebooting...', ENDL, 0
msg_kernel_not_found:   db 'No active partition found, stopping..', ENDL, 0
kernel_cluster:         dw 0

DiskAddressPacket:
    .size:              db 10h
                        db 0
    .count:             dw 0
    .offset:            dw 0
    .segment:           dw 0
    .lba:               dq 0

VBR_LOAD_SEGMENT     equ 0
VBR_LOAD_OFFSET      equ 0x7c00

times 440-($-$$) db 0

uniqueDiskID: db 0xf0, 0x99, 0xbf, 0xc4
reservedValue: dw 0

PARTITION_A:
    .isActive: db 0x00
    .first_CHS: db 0x00, 0x21, 0x00
    .type: db 0x0b
    .last_CHS: db 0xf0, 0xe4, 0xcc
    .lba: db 20, 00, 00, 00
    .sector_count: db 0xe0, 0x7f, 0xee, 0x00

times 510-($-$$) db 0
dw 0AA55h
