#pragma once
#include <stdint.h>

/* Segment selectors (byte offsets into the GDT). Low 2 bits = RPL. */
#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE   0x18       /* use | 3 from ring 3 */
#define GDT_USER_DATA   0x20
#define GDT_TSS         0x28

void gdt_init(void);
void gdt_set_tss(uint32_t base, uint32_t limit);
