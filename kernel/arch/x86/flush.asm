; Load the GDT/IDT registers. Called from C as gdt_flush(&gdtr) / idt_flush(&idtr).

[BITS 32]

global gdt_flush
gdt_flush:
    mov     eax, [esp + 4]
    lgdt    [eax]
    mov     ax, 0x10            ; kernel data selector
    mov     ds, ax
    mov     es, ax
    mov     fs, ax
    mov     gs, ax
    mov     ss, ax
    jmp     0x08:.reload_cs     ; far jump reloads CS with the kernel code selector
.reload_cs:
    ret

global idt_flush
idt_flush:
    mov     eax, [esp + 4]
    lidt    [eax]
    ret
