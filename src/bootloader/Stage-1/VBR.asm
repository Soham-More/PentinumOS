org 0x7C00
bits 16

%define ENDL 0x0D, 0x0A

jmp short code
nop

FAT_BPB:
    .oem:                    db 'MSWIN4.1'           ; 8 bytes
    .bytes_per_sector:       dw 512
    .sectors_per_cluster:    db 1
    .reserved_sectors:       dw 12h                  ; BPB + FSInfo + backup + extra?
    .fat_count:              db 2
    .dir_entries_count:      dw 0
    .total_sectors:          dw 2880                 ; no of sectors in this partition
    .media_descriptor_type:  db 0F8h                 ; F8 = partitioned hard disk
    .sectors_per_fat:        dw 0                    ; only FAT 12/16
    .sectors_per_track:      dw 63
    .heads:                  dw 255
    .hidden_sectors:         dd 0                    ; partition offset
    .large_sector_count:     dd 0                    ; no of sectors in this partition(32 bits)

FAT_EBPB:
    .sectors_per_fat:        dd 0x9e                 ; no of sectors per FAT
    .flags:                  dw 0
    .version:                dw 0
    .root_cluster:           dd 2
    .sectorFSInfo:           dw 1
    .backupBootSector:       dw 0                    ; no backup is available
    .reserved:               times 12 db 0
    .driveID:                db 80h
    .flagsNT:                db 0
    .signature:              db 29h
    .volumeID:               db 0x57, 0x01, 0xd1, 0xb8
    .volumeLable:            db 'REAL OS    '
    .systemID:               db 'FAT 32  '

code:

    pop eax

    mov si, msg_vbr
    call puts

    ; eax contains current partition offset
    ; stage-2 is at eax + 2
    add eax, 10
    mov [DiskAddressPacket.lba], eax
    mov ax, word [FAT_BPB.reserved_sectors]
    sub ax, 10
    mov [DiskAddressPacket.count], ax
    mov [DiskAddressPacket.offset], word STAGE2_LOAD_OFFSET

    call disk_read

    jmp STAGE2_LOAD_SEGMENT:STAGE2_LOAD_OFFSET

    cli
    hlt

;
; Error handlers
;

read_error:
    mov si, msg_read_failed
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

    ; make eax zero
    xor eax, eax

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

msg_vbr:                db 'VBR loaded..', ENDL, 0
msg_read_failed:        db 'VBR: disk read failed, rebooting...', ENDL, 0

DiskAddressPacket:
    .size:              db 10h
                        db 0
    .count:             dw 0
    .offset:            dw 0
    .segment:           dw 0
    .lba:               dq 0

STAGE2_LOAD_SEGMENT     equ 0
STAGE2_LOAD_OFFSET      equ 0x500

times 510-($-$$) db 0
dw 0AA55h

