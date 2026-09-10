#include "console.h"
#include "string.h"
#include "io.h"

#define COLS 80
#define ROWS 25
#define TABSTOP 8

static volatile uint16_t *const BUF = (volatile uint16_t *)0xB8000;

static uint32_t cur_row;
static uint32_t cur_col;
static uint8_t  cur_attr;

static uint16_t cell(char c)
{
    return (uint16_t)((uint8_t)c) | ((uint16_t)cur_attr << 8);
}

static void update_hw_cursor(void)
{
    uint16_t pos = (uint16_t)(cur_row * COLS + cur_col);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)(pos >> 8));
}

static void scroll(void)
{
    /* move rows 1..ROWS-1 up by one, blank the last row */
    memmove((void *)BUF, (const void *)(BUF + COLS),
            (size_t)(COLS * (ROWS - 1)) * sizeof(uint16_t));
    for (uint32_t x = 0; x < COLS; x++)
        BUF[(ROWS - 1) * COLS + x] = cell(' ');
}

static void newline(void)
{
    cur_col = 0;
    if (++cur_row >= ROWS) {
        scroll();
        cur_row = ROWS - 1;
    }
}

void console_set_color(enum vga_color fg, enum vga_color bg)
{
    cur_attr = vga_attr(fg, bg);
}

void console_clear(void)
{
    for (uint32_t i = 0; i < COLS * ROWS; i++)
        BUF[i] = cell(' ');
    cur_row = cur_col = 0;
    update_hw_cursor();
}

void console_init(void)
{
    cur_attr = vga_attr(VGA_LGRAY, VGA_BLACK);
    console_clear();
}

void console_putc(char c)
{
    switch (c) {
    case '\n':
        newline();
        break;
    case '\r':
        cur_col = 0;
        break;
    case '\b':
        if (cur_col > 0) {
            cur_col--;
            BUF[cur_row * COLS + cur_col] = cell(' ');
        }
        break;
    case '\t':
        cur_col = (cur_col + TABSTOP) & ~(TABSTOP - 1);
        if (cur_col >= COLS)
            newline();
        break;
    default:
        BUF[cur_row * COLS + cur_col] = cell(c);
        if (++cur_col >= COLS)
            newline();
        break;
    }
    update_hw_cursor();
}

void console_write(const char *s)
{
    while (*s)
        console_putc(*s++);
}
