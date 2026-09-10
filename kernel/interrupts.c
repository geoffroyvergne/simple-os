#include "interrupts.h"
#include "idt.h"
#include "pic.h"
#include "kprintf.h"
#include "vga.h"
#include "console.h"
#include "syscall.h"

static const char *const EXCEPTION_NAMES[32] = {
    "divide error", "debug", "NMI", "breakpoint",
    "overflow", "BOUND range exceeded", "invalid opcode", "device not available",
    "double fault", "coprocessor segment overrun", "invalid TSS", "segment not present",
    "stack-segment fault", "general protection fault", "page fault", "reserved (15)",
    "x87 FP exception", "alignment check", "machine check", "SIMD FP exception",
    "virtualization exception", "control protection exception", "reserved (22)", "reserved (23)",
    "reserved (24)", "reserved (25)", "reserved (26)", "reserved (27)",
    "hypervisor injection", "VMM communication", "security exception", "reserved (31)",
};

static irq_handler_t irq_handlers[16];

void irq_install_handler(uint8_t irq, irq_handler_t handler)
{
    if (irq < 16)
        irq_handlers[irq] = handler;
}

void irq_uninstall_handler(uint8_t irq)
{
    if (irq < 16)
        irq_handlers[irq] = 0;
}

void panic(const char *msg, struct registers *r)
{
    __asm__ volatile("cli");
    console_set_color(VGA_WHITE, VGA_RED);
    kprintf("\n\n*** KERNEL PANIC: %s ***\n", msg);
    if (r) {
        kprintf("int=%u err=0x%x eip=%p cs=0x%x eflags=0x%x\n",
                r->int_no, r->err_code, (void *)r->eip, r->cs, r->eflags);
        kprintf("eax=%p ebx=%p ecx=%p edx=%p\n",
                (void *)r->eax, (void *)r->ebx, (void *)r->ecx, (void *)r->edx);
        kprintf("esi=%p edi=%p ebp=%p esp=%p\n",
                (void *)r->esi, (void *)r->edi, (void *)r->ebp, (void *)r->useresp);
    }
    kprintf("system halted.\n");
    for (;;)
        __asm__ volatile("hlt");
}

void interrupt_dispatch(struct registers *r)
{
    if (r->int_no == 0x80) {
        syscall_dispatch(r);
        return;
    }

    if (r->int_no < 32) {
        if (r->int_no == 14) {          /* page fault: CR2 holds the address */
            uint32_t cr2;
            __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
            kprintf("\nfaulting address (CR2) = %p\n", (void *)cr2);
        }
        panic(EXCEPTION_NAMES[r->int_no], r);
    }

    uint8_t irq = (uint8_t)(r->int_no - PIC_IRQ_BASE);

    /* Spurious IRQ7/IRQ15: no real interrupt, do not EOI the master for 15. */
    if (irq < 16 && irq_handlers[irq])
        irq_handlers[irq](r);

    pic_send_eoi(irq);
}

void interrupts_init(void)
{
    idt_init();
    pic_remap();
}
