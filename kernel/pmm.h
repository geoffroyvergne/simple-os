#pragma once
#include <stdint.h>
#include <stddef.h>
#include "bootinfo.h"

#define PAGE_SIZE 4096u

void pmm_init(const struct bootinfo *bi);

/* Physical addresses. 0 means "out of memory". Frame 0 is never handed out. */
uint32_t pmm_alloc_frame(void);
uint32_t pmm_alloc_contiguous(size_t count);
void     pmm_free_frame(uint32_t phys);

uint32_t pmm_total_frames(void);
uint32_t pmm_used_frames(void);
uint32_t pmm_free_frames(void);
