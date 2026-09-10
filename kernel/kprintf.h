#pragma once
#include <stdarg.h>

/* Kernel formatted output. Writes to the VGA console and the COM1 serial log.
 *
 * Supported: %c %s %% %d %i %u %x %X %p %b, optional '0' flag, optional field
 * width, optional 'l'/'ll' length modifiers. No floating point. */

void kputchar(char c);
void kputs(const char *s);
int  kprintf(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
int  kvprintf(const char *fmt, va_list ap);
