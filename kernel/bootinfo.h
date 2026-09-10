#pragma once
#include <stdint.h>

/* Filled in by boot/stage2.asm (real mode) at a fixed physical address, then
 * read by the kernel before paging is enabled. Keep in sync with stage2. */

#define BOOTINFO_PHYS  0x00050000u
#define BOOTINFO_MAGIC 0xB007B007u
#define BOOTINFO_MAX_E820 64

/* One INT 15h/E820 entry (ACPI 3.0 form: 24 bytes). */
struct e820_entry {
    uint64_t base;
    uint64_t len;
    uint32_t type;          /* 1 = usable RAM, 2 = reserved, 3/4 = ACPI, 5 = bad */
    uint32_t acpi_flags;
} __attribute__((packed));

#define E820_USABLE 1

struct bootinfo {
    uint32_t magic;
    uint32_t e820_count;
    struct e820_entry e820[BOOTINFO_MAX_E820];
} __attribute__((packed));
