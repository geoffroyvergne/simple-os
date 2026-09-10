; =============================================================================
; A minimal ring-3 program. Linked into its own page-aligned .user section
; (see linker.ld) so paging_set_user() can grant CPL 3 access to exactly it.
; Talks to the kernel only through `int 0x80`.
; =============================================================================

[BITS 32]
section .user

global user_entry
user_entry:
    ; write(1, msg, msg_len)
    mov     eax, 1
    mov     ebx, 1
    mov     ecx, msg
    mov     edx, msg_len
    int     0x80

    ; exit(getpid())
    mov     eax, 3
    int     0x80
    mov     ebx, eax
    mov     eax, 0
    int     0x80

.hang:
    jmp     .hang

msg:     db "hello from ring 3 (via int 0x80)", 10
msg_len equ $ - msg
