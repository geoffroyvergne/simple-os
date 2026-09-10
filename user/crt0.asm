; C runtime startup for user programs. The kernel enters here with the user
; stack laid out as:  [esp] = argc,  [esp+4] = argv (char **).

[BITS 32]

global _start
extern main
extern exit

_start:
    mov     eax, [esp]          ; argc
    lea     edx, [esp + 4]      ; &argv[0]  -> argv
    push    edx
    push    eax
    call    main
    add     esp, 8

    push    eax                 ; exit(main(...))
    call    exit
.hang:
    jmp     .hang
