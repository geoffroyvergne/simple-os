#include "paging.h"
#include "pmm.h"
#include "string.h"
#include "kprintf.h"

#define PTE_COUNT 1024
#define MAX_MAP_FRAMES (256u * 1024)     /* 1 GiB / 4 KiB */

static uint32_t *page_directory;
static uint32_t  mapped_frames;

void paging_init(void)
{
    uint32_t frames = pmm_total_frames();
    if (frames > MAX_MAP_FRAMES)
        frames = MAX_MAP_FRAMES;

    uint32_t tables = (frames + PTE_COUNT - 1) / PTE_COUNT;

    page_directory = (uint32_t *)pmm_alloc_frame();
    memset(page_directory, 0, PAGE_SIZE);

    for (uint32_t t = 0; t < tables; t++) {
        uint32_t *table = (uint32_t *)pmm_alloc_frame();
        for (uint32_t i = 0; i < PTE_COUNT; i++) {
            uint32_t frame = t * PTE_COUNT + i;
            table[i] = (frame * PAGE_SIZE) | PTE_PRESENT | PTE_WRITE;
        }
        page_directory[t] = (uint32_t)table | PTE_PRESENT | PTE_WRITE;
    }
    mapped_frames = tables * PTE_COUNT;

    __asm__ volatile("mov %0, %%cr3" : : "r"(page_directory) : "memory");

    uint32_t cr0;
    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u;                    /* CR0.PG */
    __asm__ volatile("mov %0, %%cr0" : : "r"(cr0) : "memory");

    kprintf("paging: identity-mapped %u MiB (%u page tables), PG on\n",
            mapped_frames / 256, tables);
}

uint32_t paging_mapped_bytes(void)
{
    return mapped_frames * PAGE_SIZE;
}

uint32_t *paging_kernel_dir(void)      { return page_directory; }
uint32_t  paging_kernel_dir_phys(void) { return (uint32_t)page_directory; }
