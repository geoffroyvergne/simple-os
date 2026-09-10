; =============================================================================
; SimpleOS - Stage 2 boot loader
;
; Loaded by Stage 1 at 0x0000:0x7E00, 16-bit real mode, DL = boot drive.
;
; Job:
;   1. Load the kernel (LBA 9..) to physical 0x10000.
;   2. Enable the A20 line.
;   3. Install a flat GDT and enter 32-bit protected mode.
;   4. Jump to the kernel entry point at 0x10000.
; =============================================================================

[BITS 16]
[ORG 0x7E00]

KERNEL_SEG      equ 0x1000         ; 0x1000:0x0000 = physical 0x10000
KERNEL_OFF      equ 0x0000
KERNEL_ADDR     equ 0x10000
KERNEL_LBA      equ 9
KERNEL_SECTORS  equ 128            ; 64 KiB max kernel for now

stage2:
    xor     ax, ax
    mov     ds, ax
    mov     es, ax
    mov     ss, ax
    mov     sp, 0x7C00
    mov     [boot_drive], dl

    mov     si, msg_s2
    call    print16

    ; --- Load the kernel ---
    mov     si, kdap
    mov     ah, 0x42
    mov     dl, [boot_drive]
    int     0x13
    jc      disk_error

    ; --- Enable A20 (fast gate, port 0x92) ---
    in      al, 0x92
    or      al, 0x02
    and     al, 0xFE               ; make sure bit 0 (fast reset) stays clear
    out     0x92, al

    ; --- Enter protected mode ---
    cli
    lgdt    [gdt_descriptor]
    mov     eax, cr0
    or      eax, 1
    mov     cr0, eax
    jmp     CODE_SEG:pm_entry

; ------------------------- 16-bit helpers -------------------------
print16:
.loop:
    lodsb
    or      al, al
    jz      .done
    mov     ah, 0x0E
    mov     bx, 0x0007
    int     0x10
    jmp     .loop
.done:
    ret

disk_error:
    mov     si, msg_derr
    call    print16
.hang:
    cli
    hlt
    jmp     .hang

; ------------------------- 32-bit entry -------------------------
[BITS 32]
pm_entry:
    mov     ax, DATA_SEG
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    mov     esp, 0x00090000       ; temporary stack below the kernel image area
    jmp     KERNEL_ADDR

; ------------------------- data -------------------------
msg_s2      db "SimpleOS stage2", 13, 10, 0
msg_derr    db "S2 DISK ERR", 13, 10, 0
boot_drive  db 0

align 4
kdap:
    db  0x10
    db  0x00
    dw  KERNEL_SECTORS
    dw  KERNEL_OFF
    dw  KERNEL_SEG
    dq  KERNEL_LBA

; Flat GDT: null, 4 GiB ring-0 code, 4 GiB ring-0 data.
align 8
gdt_start:
    dq 0x0000000000000000
gdt_code:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10011010b
    db 11001111b
    db 0x00
gdt_data:
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00
gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start
