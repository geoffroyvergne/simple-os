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

void paging_set_user(uint32_t vaddr, uint32_t size)
{
    uint32_t start = vaddr & ~0xFFFu;
    uint32_t end = (vaddr + size + 0xFFFu) & ~0xFFFu;

    for (uint32_t a = start; a != end; a += PAGE_SIZE) {
        uint32_t pd = a >> 22;
        uint32_t pt = (a >> 12) & 0x3FF;
        if (!(page_directory[pd] & PTE_PRESENT))
            continue;
        uint32_t *table = (uint32_t *)(page_directory[pd] & ~0xFFFu);
        table[pt] |= PTE_USER;
        page_directory[pd] |= PTE_USER;
        __asm__ volatile("invlpg (%0)" : : "r"(a) : "memory");
    }
}

uint32_t paging_mapped_bytes(void)
{
    return mapped_frames * PAGE_SIZE;
}
