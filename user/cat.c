#include "libc.h"

/* Echoes stdin back to stdout a line at a time until an empty line. A real
 * cat needs an open()/close() syscall on the VFS -- that comes with the
 * filesystem syscalls in a later step. */
int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    char line[256];
    puts("type lines; an empty line quits.\n");
    for (;;) {
        int n = readline(line, sizeof(line));
        if (n <= 0)
            break;
        write(1, line, (size_t)n);
        putchar('\n');
    }
    return 0;
}
