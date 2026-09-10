; =============================================================================
; SimpleOS - kernel entry stub
;
; Stage 2 jumps here in 32-bit protected mode with a flat GDT already loaded.
; We zero .bss, set up a real stack, and hand control to kmain().
; =============================================================================

[BITS 32]

global _start
extern kmain

section .text.boot
_start:
    cli
    mov     esp, stack_top

    ; zero the .bss section
    extern  __bss_start
    extern  __bss_end
    mov     edi, __bss_start
    mov     ecx, __bss_end
    sub     ecx, edi
    xor     eax, eax
    rep     stosb

    call    kmain

.hang:
    cli
    hlt
    jmp     .hang

section .bss
align 16
stack_bottom:
    resb 16384
stack_top:
