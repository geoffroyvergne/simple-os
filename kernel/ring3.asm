; =============================================================================
; Ring 0 <-> ring 3 transitions with an address-space switch.
;
;   run_user(entry, user_esp, cr3)  saves the kernel CR3/ESP, switches to the
;                                   process CR3, and iret's to CPL 3.
;   user_exit(code)                 called from the SYS_exit handler; restores
;                                   the kernel CR3/ESP and returns from run_user
;                                   with `code`.
; =============================================================================

[BITS 32]

USER_CS equ 0x1B          ; GDT user code (0x18) | RPL 3
USER_DS equ 0x23          ; GDT user data (0x20) | RPL 3
KERN_DS equ 0x10

section .data
saved_kesp: dd 0
saved_cr3:  dd 0
exit_code:  dd 0

section .text

global run_user
run_user:
    pushad
    mov     [saved_kesp], esp
    mov     eax, cr3
    mov     [saved_cr3], eax

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

    mov     eax, [saved_cr3]
    mov     cr3, eax
    mov     esp, [saved_kesp]

    mov     ax, KERN_DS
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    popad
    mov     eax, [exit_code]         ; popad clobbered eax; set return value
    sti                              ; syscall gate cleared IF; kernel wants it on
    ret
