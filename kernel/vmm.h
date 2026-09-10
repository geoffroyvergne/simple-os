#pragma once
#include <stdint.h>

/* Per-process virtual address spaces. The kernel's identity map (low RAM) is
 * shared into every space by copying the kernel page-directory entries; user
 * pages live at high virtual addresses (>= 1 GiB) that the kernel dir leaves
 * empty. Page tables and directories are themselves frames from the PMM, which
 * are identity-mapped, so a space can be built while running on any CR3. */

#define USER_BASE       0x40000000u
#define USER_STACK_TOP  0x50000000u
#define USER_STACK_SIZE 0x4000u          /* 16 KiB */

uint32_t *vmm_create(void);              /* new directory (virt == phys) */
void      vmm_destroy(uint32_t *dir);    /* frees user tables + frames, then dir */

/* Map one page of physical memory into a space with user access. */
void      vmm_map_user(uint32_t *dir, uint32_t vaddr, uint32_t phys, int writable);

/* Physical address backing a user virtual address (offset included), or 0. */
uint32_t  vmm_phys(uint32_t *dir, uint32_t vaddr);

/* Copy into user memory of another address space (used to stage argv). */
void      vmm_copy_to_user(uint32_t *dir, uint32_t vaddr, const void *src, uint32_t n);
