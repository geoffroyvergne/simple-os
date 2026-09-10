#pragma once
#include <stdint.h>
#include "vga.h"

/* 80x25 VGA text-mode terminal: tracks a cursor, scrolls, updates the
 * hardware cursor, and interprets \n \r \t \b. */

void console_init(void);
void console_clear(void);
void console_set_color(enum vga_color fg, enum vga_color bg);
void console_putc(char c);
void console_write(const char *s);
