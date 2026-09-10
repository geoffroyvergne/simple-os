#include "pmm.h"
#include "string.h"
#include "kprintf.h"

/* Bitmap physical frame allocator. 1 bit per 4 KiB frame, 1 = used. The bitmap
 * itself is placed immediately after the kernel image (still in low memory,
 * which we reserve wholesale). */

extern uint8_t __kernel_end[];

static uint32_t *bitmap;
static uint32_t  bitmap_words;
static uint32_t  total_frames;
static uint32_t  used_frames;

static inline void bset(uint32_t f) { bitmap[f >> 5] |= 1u << (f & 31); }
static inline void bclr(uint32_t f) { bitmap[f >> 5] &= ~(1u << (f & 31)); }
static inline int  btest(uint32_t f) { return (bitmap[f >> 5] >> (f & 31)) & 1; }

static void reserve(uint32_t base, uint32_t end)
{
    uint32_t f0 = base / PAGE_SIZE;
    uint32_t f1 = (end + PAGE_SIZE - 1) / PAGE_SIZE;
    for (uint32_t f = f0; f < f1 && f < total_frames; f++)
        if (!btest(f)) { bset(f); used_frames++; }
}

static void release(uint64_t base, uint64_t end)
{
    if (end > (uint64_t)total_frames * PAGE_SIZE)
        end = (uint64_t)total_frames * PAGE_SIZE;
    uint32_t f0 = (uint32_t)((base + PAGE_SIZE - 1) / PAGE_SIZE);  /* round in */
    uint32_t f1 = (uint32_t)(end / PAGE_SIZE);
    for (uint32_t f = f0; f < f1; f++)
        if (btest(f)) { bclr(f); used_frames--; }
}

void pmm_init(const struct bootinfo *bi)
{
    int have_map = bi && bi->magic == BOOTINFO_MAGIC && bi->e820_count > 0;

    uint64_t ram_end = 0;
    if (have_map) {
        for (uint32_t i = 0; i < bi->e820_count; i++) {
            const struct e820_entry *e = &bi->e820[i];
            if (e->type != E820_USABLE)
                continue;
            uint64_t top = e->base + e->len;
            if (top > ram_end)
                ram_end = top;
        }
    }
    if (ram_end == 0)                       /* no usable map: assume 32 MiB */
        ram_end = 32u * 1024 * 1024;
    if (ram_end > 0xFFFFF000ULL)            /* cap the bitmap at ~4 GiB */
        ram_end = 0xFFFFF000ULL;

    total_frames  = (uint32_t)(ram_end / PAGE_SIZE);
    bitmap_words  = (total_frames + 31) / 32;
    bitmap        = (uint32_t *)(((uint32_t)__kernel_end + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1));

    memset(bitmap, 0xFF, bitmap_words * 4);   /* everything used to start */
    used_frames = total_frames;

    if (have_map) {
        for (uint32_t i = 0; i < bi->e820_count; i++) {
            const struct e820_entry *e = &bi->e820[i];
            if (e->type == E820_USABLE)
                release(e->base, e->base + e->len);
        }
    } else {
        release(0x100000, ram_end);
    }

    /* Reserve: real-mode/BIOS/VGA/bootinfo region, the kernel, and the bitmap. */
    uint32_t bitmap_end = (uint32_t)bitmap + bitmap_words * 4;
    reserve(0, bitmap_end > 0x100000 ? bitmap_end : 0x100000);

    kprintf("pmm: %u MiB RAM, %u frames, %u free\n",
            total_frames / 256, total_frames, pmm_free_frames());
}

uint32_t pmm_alloc_frame(void)
{
    for (uint32_t w = 0; w < bitmap_words; w++) {
        if (bitmap[w] == 0xFFFFFFFFu)
            continue;
        for (uint32_t b = 0; b < 32; b++) {
            uint32_t f = w * 32 + b;
            if (f >= total_frames)
                return 0;
            if (!btest(f)) {
                bset(f);
                used_frames++;
                return f * PAGE_SIZE;
            }
        }
    }
    return 0;
}

uint32_t pmm_alloc_contiguous(size_t count)
{
    if (count == 0)
        return 0;
    uint32_t run = 0, start = 0;
    for (uint32_t f = 1; f < total_frames; f++) {   /* skip frame 0 */
        if (!btest(f)) {
            if (run == 0)
                start = f;
            if (++run == count) {
                for (uint32_t i = start; i < start + count; i++) {
                    bset(i);
                    used_frames++;
                }
                return start * PAGE_SIZE;
            }
        } else {
            run = 0;
        }
    }
    return 0;
}

void pmm_free_frame(uint32_t phys)
{
    uint32_t f = phys / PAGE_SIZE;
    if (f < total_frames && btest(f)) {
        bclr(f);
        used_frames--;
    }
}

uint32_t pmm_total_frames(void) { return total_frames; }
uint32_t pmm_used_frames(void)  { return used_frames; }
uint32_t pmm_free_frames(void)  { return total_frames - used_frames; }
