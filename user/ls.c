#include "libc.h"

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    struct dirent d;
    int n = 0;
    for (int i = 0; readdir(i, &d) == 0; i++) {
        puts(d.name);
        for (int s = (int)strlen(d.name); s < 16; s++)
            putchar(' ');
        putint((int)d.size);
        puts(" bytes\n");
        n++;
    }
    if (n == 0)
        puts("(empty)\n");
    return 0;
}
