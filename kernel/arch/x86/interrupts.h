#pragma once
#include <stdint.h>

/* Layout must match int_common in isr.asm exactly. */
struct registers {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp_unused, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags, useresp, ss;
};

typedef void (*irq_handler_t)(struct registers *);

void interrupts_init(void);                 /* GDT already set up by caller */
void irq_install_handler(uint8_t irq, irq_handler_t handler);
void irq_uninstall_handler(uint8_t irq);

void interrupt_dispatch(struct registers *r);   /* called from asm */

__attribute__((noreturn)) void panic(const char *msg, struct registers *r);
