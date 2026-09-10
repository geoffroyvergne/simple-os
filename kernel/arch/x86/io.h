#pragma once
#include <stdint.h>

static inline void outb(uint16_t port, uint8_t value)
{
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/* Block string I/O for PIO device transfers (words = 16-bit units). */
static inline void insw(uint16_t port, void *addr, uint32_t words)
{
    __asm__ volatile("rep insw" : "+D"(addr), "+c"(words) : "d"(port) : "memory");
}

static inline void outsw(uint16_t port, const void *addr, uint32_t words)
{
    __asm__ volatile("rep outsw" : "+S"(addr), "+c"(words) : "d"(port));
}
