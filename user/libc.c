#include "libc.h"

#define SYS_exit     0
#define SYS_write    1
#define SYS_read     2
#define SYS_getpid   3
#define SYS_yield    4
#define SYS_open     5
#define SYS_close    6
#define SYS_lseek    7
#define SYS_readdir  8
#define SYS_stat     9
#define SYS_spawn    10
#define SYS_unlink   11
#define SYS_sysinfo  12
#define SYS_reboot   13
#define SYS_wait     14
#define SYS_sleep    15

static int sc(int n, int a, int b, int c_)
{
    int ret;
    __asm__ volatile("int $0x80"
                     : "=a"(ret)
                     : "a"(n), "b"(a), "c"(b), "d"(c_)
                     : "memory");
    return ret;
}

void exit(int code)      { sc(SYS_exit, code, 0, 0); for (;;) ; }
void reboot(void)        { sc(SYS_reboot, 0, 0, 0); for (;;) ; }
int  getpid(void)        { return sc(SYS_getpid, 0, 0, 0); }
int  write(int fd, const void *b, size_t n) { return sc(SYS_write, fd, (int)b, (int)n); }
int  read(int fd, void *b, size_t n)        { return sc(SYS_read, fd, (int)b, (int)n); }
int  open(const char *p, int f)             { return sc(SYS_open, (int)p, f, 0); }
int  close(int fd)                          { return sc(SYS_close, fd, 0, 0); }
int  lseek(int fd, unsigned off)            { return sc(SYS_lseek, fd, (int)off, 0); }
int  readdir(int i, struct dirent *o)       { return sc(SYS_readdir, i, (int)o, 0); }
int  stat(const char *p, struct statbuf *o) { return sc(SYS_stat, (int)p, (int)o, 0); }
int  spawn(const char *p, char **argv)      { return sc(SYS_spawn, (int)p, (int)argv, 0); }
int  wait(int pid, int *code)               { return sc(SYS_wait, pid, (int)code, 0); }
void sleep(unsigned ms)                     { sc(SYS_sleep, (int)ms, 0, 0); }
int  unlink(const char *p)                  { return sc(SYS_unlink, (int)p, 0, 0); }
int  sysinfo(struct sysinfo *o)             { return sc(SYS_sysinfo, (int)o, 0, 0); }

size_t strlen(const char *s)
{
    size_t n = 0;
    while (s[n])
        n++;
    return n;
}

int strcmp(const char *a, const char *b)
{
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

void *memcpy(void *d, const void *s, size_t n)
{
    unsigned char *dp = d;
    const unsigned char *sp = s;
    while (n--)
        *dp++ = *sp++;
    return d;
}

void putchar(char c)     { write(1, &c, 1); }
void puts(const char *s) { write(1, s, strlen(s)); }

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
