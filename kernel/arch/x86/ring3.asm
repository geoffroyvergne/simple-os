; =============================================================================
; Ring 0 <-> ring 3 transitions with an address-space switch.
;
;   run_user(entry, user_esp, cr3)  saves the kernel CR3/ESP on a small nesting
;                                   stack, switches to the process CR3, and
;                                   iret's to CPL 3.
;   user_exit(code)                 called from the SYS_exit handler; pops the
;                                   nesting stack, restores CR3/ESP, and returns
;                                   from the matching run_user with `code`.
;
; The nesting stack lets a process spawn another (the parent is suspended inside
; its SYS_spawn until the child exits).
; =============================================================================

[BITS 32]

USER_CS   equ 0x1B        ; GDT user code (0x18) | RPL 3
USER_DS   equ 0x23        ; GDT user data (0x20) | RPL 3
KERN_DS   equ 0x10
MAX_DEPTH equ 8

section .data
ctx_kesp:  times MAX_DEPTH dd 0
ctx_cr3:   times MAX_DEPTH dd 0
ctx_depth: dd 0
exit_code: dd 0

section .text

global run_user
run_user:
    cli
    pushad

    mov     eax, [ctx_depth]
    mov     [ctx_kesp + eax * 4], esp
    mov     edx, cr3
    mov     [ctx_cr3 + eax * 4], edx
    inc     eax
    mov     [ctx_depth], eax

    mov     ecx, [esp + 32 + 4]      ; entry
    mov     edx, [esp + 32 + 8]      ; user esp
    mov     eax, [esp + 32 + 12]     ; process cr3
    mov     cr3, eax

    mov     ax, USER_DS
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax

    push    USER_DS                  ; SS
    push    edx                      ; ESP
    pushfd
    pop     eax
    or      eax, 0x200               ; IF on in user
    push    eax                      ; EFLAGS
    push    USER_CS                  ; CS
    push    ecx                      ; EIP
    iret

global user_exit
user_exit:
    mov     eax, [esp + 4]
    mov     [exit_code], eax

    mov     eax, [ctx_depth]
    dec     eax
    mov     [ctx_depth], eax

    mov     edx, [ctx_cr3 + eax * 4]
    mov     cr3, edx
    mov     esp, [ctx_kesp + eax * 4]

    mov     ax, KERN_DS
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    popad
    mov     eax, [exit_code]         ; popad clobbered eax; set return value
    sti                              ; syscall gate cleared IF; kernel wants it on
    ret
