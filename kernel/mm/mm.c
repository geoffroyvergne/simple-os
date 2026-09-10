#include "mm/mm.h"
#include "mm/pmm.h"
#include "arch/x86/paging.h"
#include "mm/kheap.h"
#include "lib/kprintf.h"

static const struct bootinfo *bi;

const struct bootinfo *bootinfo_get(void)
{
    return bi;
}

void mm_init(void)
{
    bi = (const struct bootinfo *)BOOTINFO_PHYS;
    pmm_init(bi);
    paging_init();
    kheap_init();
}

void mm_report(void)
{
    uint32_t total = pmm_total_frames();
    uint32_t used = pmm_used_frames();

    kprintf("physical: %u MiB total, %u KiB used, %u MiB free  (%u frames)\n",
            total / 256, used * 4, pmm_free_frames() / 256, total);
    kprintf("mapped:   %u MiB identity\n", paging_mapped_bytes() / (1024 * 1024));

    size_t ht, hu, hb;
    kheap_stats(&ht, &hu, &hb);
    kprintf("kheap:    %u KiB total, %u KiB used, %u KiB largest free block\n",
            (uint32_t)ht / 1024, (uint32_t)hu / 1024, (uint32_t)hb / 1024);
}
