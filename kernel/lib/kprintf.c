#include "lib/kprintf.h"
#include "term/console.h"
#include "drivers/serial.h"
#include <stdint.h>
#include <stdbool.h>

/* NOTE: arithmetic here is deliberately 32-bit. i686 has no hardware 64-bit
 * divide, and we do not link compiler-rt, so a u64 %/ would need __udivdi3.
 * The 'l'/'ll' modifiers are parsed for source compatibility but values are
 * currently narrowed to 32 bits. Revisit once we have a builtins library. */

void kputchar(char c)
{
    console_putc(c);
    serial_putc(c);
}

void kputs(const char *s)
{
    while (*s)
        kputchar(*s++);
}

static int emit_pad(int count, char pad)
{
    for (int i = 0; i < count; i++)
        kputchar(pad);
    return count > 0 ? count : 0;
}

static int emit_str(const char *s, int width, char pad)
{
    int len = 0;
    for (const char *p = s; *p; p++)
        len++;
    int n = emit_pad(width - len, pad);
    while (*s) {
        kputchar(*s++);
        n++;
    }
    return n;
}

static int emit_uint(uint32_t value, unsigned base, bool upper, int width, char pad)
{
    char buf[35];
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    int i = 0;

    if (value == 0)
        buf[i++] = '0';
    while (value) {
        buf[i++] = digits[value % base];
        value /= base;
    }

    int n = emit_pad(width - i, pad);
    while (i--) {
        kputchar(buf[i]);
        n++;
    }
    return n;
}

static int emit_int(int32_t value, int width, char pad)
{
    if (value < 0) {
        kputchar('-');
        return 1 + emit_uint((uint32_t)(-(int64_t)value), 10, false,
                             width > 0 ? width - 1 : 0, pad);
    }
    return emit_uint((uint32_t)value, 10, false, width, pad);
}

static uint32_t next_uint(va_list *ap, int lng)
{
    if (lng >= 2)
        return (uint32_t)va_arg(*ap, unsigned long long);
    if (lng == 1)
        return (uint32_t)va_arg(*ap, unsigned long);
    return va_arg(*ap, unsigned int);
}

int kvprintf(const char *fmt, va_list ap)
{
    int written = 0;

    for (const char *f = fmt; *f; f++) {
        if (*f != '%') {
            kputchar(*f);
            written++;
            continue;
        }
        f++;

        char pad = ' ';
        int width = 0;
        int lng = 0;

        if (*f == '0') {
            pad = '0';
            f++;
        }
        while (*f >= '0' && *f <= '9')
            width = width * 10 + (*f++ - '0');
        while (*f == 'l') {
            lng++;
            f++;
        }

        switch (*f) {
        case 'c':
            kputchar((char)va_arg(ap, int));
            written++;
            break;
        case 's': {
            const char *s = va_arg(ap, const char *);
            written += emit_str(s ? s : "(null)", width, pad);
            break;
        }
        case 'd':
        case 'i': {
            int32_t v = lng >= 2 ? (int32_t)va_arg(ap, long long)
                      : lng == 1 ? (int32_t)va_arg(ap, long)
                                 : va_arg(ap, int);
            written += emit_int(v, width, pad);
            break;
        }
        case 'u':
            written += emit_uint(next_uint(&ap, lng), 10, false, width, pad);
            break;
        case 'x':
            written += emit_uint(next_uint(&ap, lng), 16, false, width, pad);
            break;
        case 'X':
            written += emit_uint(next_uint(&ap, lng), 16, true, width, pad);
            break;
        case 'b':
            written += emit_uint(next_uint(&ap, lng), 2, false, width, pad);
            break;
        case 'p': {
            uintptr_t v = (uintptr_t)va_arg(ap, void *);
            kputchar('0');
            kputchar('x');
            written += 2 + emit_uint((uint32_t)v, 16, false,
                                     (int)sizeof(void *) * 2, '0');
            break;
        }
        case '%':
            kputchar('%');
            written++;
            break;
        case '\0':
            return written;
        default:
            kputchar('%');
            kputchar(*f);
            written += 2;
            break;
        }
    }
    return written;
}

int kprintf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    int n = kvprintf(fmt, ap);
    va_end(ap);
    return n;
}
