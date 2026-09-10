; =============================================================================
; Context switching and the ring-0 -> ring-3 entry.
;
;   switch_context(&prev->ksp, next->ksp, next->cr3)
;       Saves callee-saved registers and ESP of the current kernel thread into
;       *prev_ksp, loads the next thread's page directory and kernel stack, and
;       returns into wherever that thread last stopped.
;
;   enter_user(entry, user_esp)
;       Builds an iret frame and drops to CPL 3. Used once per process, from its
;       bootstrap. It never returns; the process leaves ring 3 only via a trap.
; =============================================================================

[BITS 32]

USER_CS equ 0x1B          ; GDT user code (0x18) | RPL 3
USER_DS equ 0x23          ; GDT user data (0x20) | RPL 3

section .text

global switch_context
switch_context:
    push    ebx
    push    esi
    push    edi
    push    ebp

    mov     eax, [esp + 20]         ; &prev->ksp
    mov     [eax], esp              ; save current kernel esp

    mov     eax, [esp + 28]         ; next->cr3
    mov     cr3, eax                ; kernel mappings are identical across dirs
    mov     esp, [esp + 24]         ; next->ksp

    pop     ebp
    pop     edi
    pop     esi
    pop     ebx
    ret

global enter_user
enter_user:
    mov     ecx, [esp + 4]          ; entry
    mov     edx, [esp + 8]          ; user esp

    mov     ax, USER_DS
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax

    push    USER_DS                 ; SS
    push    edx                     ; ESP
    pushfd
    pop     eax
    or      eax, 0x200              ; IF
    push    eax                     ; EFLAGS
    push    USER_CS                 ; CS
    push    ecx                     ; EIP
    iret
