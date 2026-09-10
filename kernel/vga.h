#pragma once
#include <stdint.h>

/* 80x25 VGA text-mode helpers. This is deliberately tiny for Step 1; Step 2
 * replaces it with a scrolling terminal and kprintf. */

enum vga_color {
    VGA_BLACK = 0,  VGA_BLUE = 1,   VGA_GREEN = 2,   VGA_CYAN = 3,
    VGA_RED = 4,    VGA_MAGENTA = 5, VGA_BROWN = 6,  VGA_LGRAY = 7,
    VGA_DGRAY = 8,  VGA_LBLUE = 9,  VGA_LGREEN = 10, VGA_LCYAN = 11,
    VGA_LRED = 12,  VGA_LMAGENTA = 13, VGA_YELLOW = 14, VGA_WHITE = 15,
};

static inline uint8_t vga_attr(enum vga_color fg, enum vga_color bg)
{
    return (uint8_t)(fg | (bg << 4));
}

void vga_clear(uint8_t attr);
void vga_puts_at(int row, int col, const char *s, uint8_t attr);
