#pragma once
#include <stdint.h>

#define PTE_PRESENT 0x1
#define PTE_WRITE   0x2
#define PTE_USER    0x4

/* Step 5: build one page directory that identity-maps all detected RAM (capped
 * at 1 GiB) with 4 KiB pages, then turn paging on. Per-process address spaces
 * come later. */
void paging_init(void);

/* Grant CPL 3 access to an existing identity-mapped range (sets PTE_USER on
 * the covering pages and their page-directory entries). */
void paging_set_user(uint32_t vaddr, uint32_t size);

uint32_t paging_mapped_bytes(void);
