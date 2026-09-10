#include "vmm.h"
#include "pmm.h"
#include "paging.h"
#include "string.h"

#define P   PTE_PRESENT
#define RW  PTE_WRITE
#define US  PTE_USER
#define KERNEL_PDES 256          /* PDEs 0..255 cover the low 1 GiB (kernel) */

uint32_t *vmm_create(void)
{
    uint32_t *dir = (uint32_t *)pmm_alloc_frame();
    if (!dir)
        return 0;
    uint32_t *kdir = paging_kernel_dir();

    for (int i = 0; i < KERNEL_PDES; i++)
        dir[i] = kdir[i];                 /* share kernel page tables */
    for (int i = KERNEL_PDES; i < 1024; i++)
        dir[i] = 0;
    return dir;
}

void vmm_map_user(uint32_t *dir, uint32_t vaddr, uint32_t phys, int writable)
{
    uint32_t pd = vaddr >> 22;
    uint32_t pt = (vaddr >> 12) & 0x3FF;

    if (!(dir[pd] & P)) {
        uint32_t table = pmm_alloc_frame();
        memset((void *)table, 0, PAGE_SIZE);
        dir[pd] = table | P | RW | US;
    }
    uint32_t *table = (uint32_t *)(dir[pd] & ~0xFFFu);
    table[pt] = (phys & ~0xFFFu) | P | US | (writable ? RW : 0);
}

uint32_t vmm_phys(uint32_t *dir, uint32_t vaddr)
{
    uint32_t pd = vaddr >> 22;
    uint32_t pt = (vaddr >> 12) & 0x3FF;
    if (!(dir[pd] & P))
        return 0;
    uint32_t *table = (uint32_t *)(dir[pd] & ~0xFFFu);
    if (!(table[pt] & P))
        return 0;
    return (table[pt] & ~0xFFFu) | (vaddr & 0xFFFu);
}

void vmm_copy_to_user(uint32_t *dir, uint32_t vaddr, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    while (n) {
        uint32_t phys = vmm_phys(dir, vaddr);
        uint32_t within = vaddr & 0xFFFu;
        uint32_t k = PAGE_SIZE - within;
        if (k > n)
            k = n;
        memcpy((void *)phys, s, k);
        s += k;
        vaddr += k;
        n -= k;
    }
}

void vmm_destroy(uint32_t *dir)
{
    for (int i = KERNEL_PDES; i < 1024; i++) {
        if (!(dir[i] & P))
            continue;
        uint32_t *table = (uint32_t *)(dir[i] & ~0xFFFu);
        for (int j = 0; j < 1024; j++)
            if (table[j] & P)
                pmm_free_frame(table[j] & ~0xFFFu);
        pmm_free_frame(dir[i] & ~0xFFFu);
    }
    pmm_free_frame((uint32_t)dir);
}
