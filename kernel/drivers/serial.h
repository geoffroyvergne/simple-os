#pragma once

/* Minimal COM1 (0x3F8) serial output, used for a boot log we can capture when
 * running QEMU headless (`make run-serial`). */

void serial_init(void);
void serial_putc(char c);
void serial_write(const char *s);
