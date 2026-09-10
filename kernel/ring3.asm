; =============================================================================
; Ring 0 <-> ring 3 transitions.
;
;   run_usermode(eip, esp)  builds an iret frame and drops to CPL 3. It only
;                           returns when the user calls SYS_exit.
;   usermode_exit(code)     called from the syscall handler; unwinds straight
;                           back to run_usermode's caller with `code`.
; =============================================================================

[BITS 32]

USER_CS equ 0x1B          ; GDT user code (0x18) | RPL 3
USER_DS equ 0x23          ; GDT user data (0x20) | RPL 3
KERN_DS equ 0x10

section .data
saved_kesp: dd 0
exit_code:  dd 0

section .text

global run_usermode
run_usermode:
    pushad
    mov     [saved_kesp], esp

    mov     ecx, [esp + 32 + 4]      ; arg0: user eip
    mov     edx, [esp + 32 + 8]      ; arg1: user esp

    mov     ax, USER_DS
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax

    push    USER_DS                  ; SS
    push    edx                      ; ESP
    pushfd
    pop     eax
    or      eax, 0x200               ; set IF in the user EFLAGS
    push    eax                      ; EFLAGS
    push    USER_CS                  ; CS
    push    ecx                      ; EIP
    iret

global usermode_exit
usermode_exit:
    mov     eax, [esp + 4]           ; exit code
    mov     [exit_code], eax

    mov     esp, [saved_kesp]
    mov     ax, KERN_DS
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    popad
    mov     eax, [exit_code]         ; popad clobbered eax; set the return value
    ret
