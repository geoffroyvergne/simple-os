#include "libc.h"

#define SYS_exit   0
#define SYS_write  1
#define SYS_read   2
#define SYS_getpid 3

static int syscall3(int n, int a, int b, int c)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(n), "b"(a), "c"(b), "d"(c)
                     : "memory");
    return ret;
}

void exit(int code)
{
    syscall3(SYS_exit, code, 0, 0);
    for (;;)
        ;
}

int getpid(void)               { return syscall3(SYS_getpid, 0, 0, 0); }
int write(int fd, const void *b, size_t n) { return syscall3(SYS_write, fd, (int)b, (int)n); }
int read(int fd, void *b, size_t n)        { return syscall3(SYS_read, fd, (int)b, (int)n); }

size_t strlen(const char *s)
{
    size_t n = 0;
    while (s[n])
        n++;
    return n;
}

void putchar(char c)          { write(1, &c, 1); }
void puts(const char *s)      { write(1, s, strlen(s)); }

void putint(int v)
{
    char buf[12];
    int i = sizeof(buf);
    unsigned u = v < 0 ? (unsigned)-v : (unsigned)v;

    buf[--i] = '\0';
    do {
        buf[--i] = (char)('0' + u % 10);
        u /= 10;
    } while (u);
    if (v < 0)
        buf[--i] = '-';
    puts(&buf[i]);
}

int readline(char *buf, size_t n)
{
    return read(0, buf, n);
}
