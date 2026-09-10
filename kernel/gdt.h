#pragma once
#include <stdint.h>

/* Segment selectors (byte offsets into the GDT). */
#define GDT_KERNEL_CODE 0x08
#define GDT_KERNEL_DATA 0x10
#define GDT_USER_CODE   0x18
#define GDT_USER_DATA   0x20

void gdt_init(void);
