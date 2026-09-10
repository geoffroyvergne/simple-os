#include "tss.h"
#include "gdt.h"
#include "string.h"

struct tss_entry {
    uint32_t prev_tss;
    uint32_t esp0, ss0;
    uint32_t esp1, ss1;
    uint32_t esp2, ss2;
    uint32_t cr3;
    uint32_t eip, eflags;
    uint32_t eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint32_t es, cs, ss, ds, fs, gs;
    uint32_t ldt;
    uint16_t trap, iomap_base;
} __attribute__((packed));

_Static_assert(sizeof(struct tss_entry) == 104, "i386 TSS is 104 bytes");

static struct tss_entry tss;
static uint8_t kstack[8192] __attribute__((aligned(16)));

uint32_t tss_kstack_top(void)
{
    return (uint32_t)(kstack + sizeof(kstack));
}

void tss_set_esp0(uint32_t esp0)
{
    tss.esp0 = esp0;
}

void tss_init(void)
{
    memset(&tss, 0, sizeof(tss));
    tss.ss0 = GDT_KERNEL_DATA;
    tss.esp0 = tss_kstack_top();
    tss.iomap_base = sizeof(struct tss_entry);   /* no I/O bitmap */

    gdt_set_tss((uint32_t)&tss, sizeof(struct tss_entry) - 1);
    __asm__ volatile("ltr %%ax" : : "a"((uint16_t)GDT_TSS));
}
