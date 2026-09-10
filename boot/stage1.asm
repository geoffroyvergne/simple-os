; =============================================================================
; SimpleOS - Stage 1 boot loader (Master Boot Record)
;
; The BIOS loads this 512-byte sector to physical address 0x7C00 and jumps to
; it in 16-bit real mode with DL = boot drive number.
;
; Job: load Stage 2 from disk (LBA 1..8) to 0x0000:0x7E00 and jump to it.
; =============================================================================

[BITS 16]
[ORG 0x7C00]

STAGE2_SEG      equ 0x0000
STAGE2_OFF      equ 0x7E00
STAGE2_LBA      equ 1
STAGE2_SECTORS  equ 8

start:
    cli
    xor     ax, ax
    mov     ds, ax
    mov     es, ax
    mov     ss, ax
    mov     sp, 0x7C00          ; stack grows down from just below us
    sti

    mov     [boot_drive], dl

    ; --- Load Stage 2 via INT 13h AH=42h (extended read, LBA) ---
    mov     si, dap
    mov     ah, 0x42
    mov     dl, [boot_drive]
    int     0x13
    jc      disk_error

    mov     dl, [boot_drive]    ; hand the boot drive to Stage 2
    jmp     STAGE2_SEG:STAGE2_OFF

disk_error:
    mov     si, msg_err
.putc:
    lodsb
    or      al, al
    jz      .hang
    mov     ah, 0x0E
    mov     bx, 0x0007
    int     0x10
    jmp     .putc
.hang:
    cli
    hlt
    jmp     .hang

msg_err     db "S1 DISK ERR", 0
boot_drive  db 0

; Disk Address Packet for the extended read
align 4
dap:
    db  0x10                    ; DAP size
    db  0x00                    ; reserved
    dw  STAGE2_SECTORS          ; number of sectors to transfer
    dw  STAGE2_OFF              ; destination offset
    dw  STAGE2_SEG              ; destination segment
    dq  STAGE2_LBA              ; starting LBA

    times 510 - ($ - $$) db 0
    dw 0xAA55
