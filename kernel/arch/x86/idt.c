#include "arch/x86/idt.h"
#include "arch/x86/gdt.h"
#include "lib/string.h"

struct idt_entry {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t  zero;
    uint8_t  flags;
    uint16_t offset_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr   idtr;

extern void idt_flush(uint32_t idtr_addr);
extern void *isr_stub_table[48];
extern void isr128(void);

void idt_set_gate(uint8_t vec, uint32_t handler, uint16_t sel, uint8_t flags)
{
    idt[vec].offset_low  = handler & 0xFFFF;
    idt[vec].selector    = sel;
    idt[vec].zero        = 0;
    idt[vec].flags       = flags;
    idt[vec].offset_high = (handler >> 16) & 0xFFFF;
}

void idt_init(void)
{
    memset(idt, 0, sizeof(idt));

    /* 0x8E = present, DPL0, 32-bit interrupt gate. */
    for (int i = 0; i < 48; i++)
        idt_set_gate((uint8_t)i, (uint32_t)isr_stub_table[i],
                     GDT_KERNEL_CODE, 0x8E);

    /* 0xEE = present, DPL 3, 32-bit interrupt gate: callable from ring 3. */
    idt_set_gate(0x80, (uint32_t)isr128, GDT_KERNEL_CODE, 0xEE);

    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uint32_t)&idt;
    idt_flush((uint32_t)&idtr);
}
