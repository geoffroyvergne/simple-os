#include "libc.h"

static void hex2(unsigned v)
{
    const char *h = "0123456789abcdef";
    putchar(h[(v >> 4) & 0xF]);
    putchar(h[v & 0xF]);
}

static void hex8(unsigned v)
{
    for (int s = 28; s >= 0; s -= 4)
        putchar("0123456789abcdef"[(v >> s) & 0xF]);
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        puts("usage: hexdump <file>\n");
        return 1;
    }
    int fd = open(argv[1], O_RDONLY);
    if (fd < 0) {
        puts("hexdump: not found\n");
        return 1;
    }

    unsigned char buf[16];
    unsigned off = 0;
    int n;
    while ((n = read(fd, buf, sizeof(buf))) > 0) {
        hex8(off);
        puts("  ");
        for (int i = 0; i < 16; i++) {
            if (i < n)
                hex2(buf[i]);
            else
                puts("  ");
            putchar(' ');
        }
        puts(" |");
        for (int i = 0; i < n; i++)
            putchar(buf[i] >= 0x20 && buf[i] < 0x7F ? (char)buf[i] : '.');
        puts("|\n");
        off += (unsigned)n;
    }
    close(fd);
    return 0;
}
