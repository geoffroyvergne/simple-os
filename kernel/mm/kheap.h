#pragma once
#include <stddef.h>

void  kheap_init(void);
void *kmalloc(size_t size);
void *kcalloc(size_t nmemb, size_t size);
void *krealloc(void *ptr, size_t size);
void  kfree(void *ptr);

void kheap_stats(size_t *total, size_t *used, size_t *largest_free);
