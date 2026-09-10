#include "kheap.h"
#include "pmm.h"
#include "string.h"
#include "kprintf.h"
#include "interrupts.h"

/* Implicit free-list heap over a fixed, physically-contiguous region taken from
 * the PMM at init. RAM is identity-mapped, so contiguous physical == contiguous
 * virtual. First-fit, split on alloc, full coalesce pass on free. */

#define HEAP_FRAMES 1024            /* 4 MiB */
#define ALIGN       16
#define MIN_SPLIT   (sizeof(struct block) + ALIGN)

struct block {
    uint32_t size;                 /* total bytes incl. this header */
    uint32_t free;
    uint32_t pad[2];               /* keep header 16 bytes -> payload 16-aligned */
};

static uint8_t *heap_base;
static uint32_t heap_size;

static struct block *first(void) { return (struct block *)heap_base; }
static struct block *next(struct block *b) { return (struct block *)((uint8_t *)b + b->size); }
static int in_heap(struct block *b) { return (uint8_t *)b < heap_base + heap_size; }

void kheap_init(void)
{
    uint32_t phys = pmm_alloc_contiguous(HEAP_FRAMES);
    if (!phys)
        panic("kheap: no contiguous region for the heap", 0);

    heap_base = (uint8_t *)phys;
    heap_size = HEAP_FRAMES * PAGE_SIZE;

    struct block *b = first();
    b->size = heap_size;
    b->free = 1;

    kprintf("kheap: %u KiB at %p\n", heap_size / 1024, heap_base);
}

static void coalesce(void)
{
    for (struct block *b = first(); in_heap(b); b = next(b)) {
        while (b->free) {
            struct block *n = next(b);
            if (!in_heap(n) || !n->free)
                break;
            b->size += n->size;
        }
    }
}

void *kmalloc(size_t size)
{
    if (size == 0)
        return 0;

    size_t need = (size + sizeof(struct block) + (ALIGN - 1)) & ~(size_t)(ALIGN - 1);

    for (struct block *b = first(); in_heap(b); b = next(b)) {
        if (!b->free || b->size < need)
            continue;

        if (b->size >= need + MIN_SPLIT) {
            struct block *rest = (struct block *)((uint8_t *)b + need);
            rest->size = b->size - need;
            rest->free = 1;
            b->size = need;
        }
        b->free = 0;
        return b + 1;
    }
    return 0;
}

void *kcalloc(size_t nmemb, size_t size)
{
    size_t total = nmemb * size;
    void *p = kmalloc(total);
    if (p)
        memset(p, 0, total);
    return p;
}

void kfree(void *ptr)
{
    if (!ptr)
        return;
    struct block *b = (struct block *)ptr - 1;
    b->free = 1;
    coalesce();
}

void *krealloc(void *ptr, size_t size)
{
    if (!ptr)
        return kmalloc(size);
    if (size == 0) {
        kfree(ptr);
        return 0;
    }
    struct block *b = (struct block *)ptr - 1;
    size_t have = b->size - sizeof(struct block);
    if (have >= size)
        return ptr;

    void *n = kmalloc(size);
    if (n) {
        memcpy(n, ptr, have);
        kfree(ptr);
    }
    return n;
}

void kheap_stats(size_t *total, size_t *used, size_t *largest_free)
{
    size_t u = 0, big = 0;
    for (struct block *b = first(); in_heap(b); b = next(b)) {
        if (b->free) {
            if (b->size > big)
                big = b->size;
        } else {
            u += b->size;
        }
    }
    if (total)        *total = heap_size;
    if (used)         *used = u;
    if (largest_free) *largest_free = big;
}
