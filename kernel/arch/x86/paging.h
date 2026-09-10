#pragma once
#include <stdint.h>

#define PTE_PRESENT 0x1
#define PTE_WRITE   0x2
#define PTE_USER    0x4

/* Step 5: build one page directory that identity-maps all detected RAM (capped
 * at 1 GiB) with 4 KiB pages, then turn paging on. Per-process address spaces
 * come later. */
void paging_init(void);

uint32_t paging_mapped_bytes(void);

/* The single kernel page directory (identity map of low RAM). Every process
 * address space copies its PDEs so kernel mappings are always present. */
uint32_t *paging_kernel_dir(void);
uint32_t  paging_kernel_dir_phys(void);
