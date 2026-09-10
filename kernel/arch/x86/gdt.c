#include "arch/x86/gdt.h"

/* A kernel-owned flat GDT: null, ring-0 code/data, ring-3 code/data, and a TSS
 * slot filled in later by tss_init(). */

struct gdt_entry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
} __attribute__((packed));

struct gdt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct gdt_entry gdt[6];
static struct gdt_ptr   gdtr;

extern void gdt_flush(uint32_t gdtr_addr);

static void set_entry(int i, uint32_t base, uint32_t limit,
                      uint8_t access, uint8_t flags)
{
    gdt[i].limit_low   = limit & 0xFFFF;
    gdt[i].base_low    = base & 0xFFFF;
    gdt[i].base_mid    = (base >> 16) & 0xFF;
    gdt[i].access      = access;
    gdt[i].granularity = ((limit >> 16) & 0x0F) | (flags & 0xF0);
    gdt[i].base_high   = (base >> 24) & 0xFF;
}

void gdt_init(void)
{
    /* access: P=1, DPL, S=1, then type bits. flags: G=1 (4KiB), D/B=1 (32-bit). */
    set_entry(0, 0, 0, 0, 0);                         /* null */
    set_entry(1, 0, 0xFFFFF, 0x9A, 0xC0);             /* kernel code, DPL0 */
    set_entry(2, 0, 0xFFFFF, 0x92, 0xC0);             /* kernel data, DPL0 */
    set_entry(3, 0, 0xFFFFF, 0xFA, 0xC0);             /* user code,   DPL3 */
    set_entry(4, 0, 0xFFFFF, 0xF2, 0xC0);             /* user data,   DPL3 */

    gdtr.limit = sizeof(gdt) - 1;
    gdtr.base  = (uint32_t)&gdt;
    gdt_flush((uint32_t)&gdtr);
}

void gdt_set_tss(uint32_t base, uint32_t limit)
{
    /* access 0x89 = present, DPL0, 32-bit TSS (available). Byte granularity. */
    set_entry(5, base, limit, 0x89, 0x00);
}
