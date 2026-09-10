#include "vga.h"

#define VGA_COLS 80
#define VGA_ROWS 25

static volatile uint16_t *const VGA = (volatile uint16_t *)0xB8000;

static uint16_t cell(char c, uint8_t attr)
{
    return (uint16_t)((uint8_t)c) | ((uint16_t)attr << 8);
}

void vga_clear(uint8_t attr)
{
    for (int i = 0; i < VGA_COLS * VGA_ROWS; i++)
        VGA[i] = cell(' ', attr);
}

void vga_puts_at(int row, int col, const char *s, uint8_t attr)
{
    if (row < 0 || row >= VGA_ROWS)
        return;
    int i = row * VGA_COLS + col;
    int end = (row + 1) * VGA_COLS;
    while (*s && i < end)
        VGA[i++] = cell(*s++, attr);
}
