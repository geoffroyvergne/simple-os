#include "libc.h"

int main(int argc, char **argv)
{
    for (int i = 1; i < argc; i++) {
        if (i > 1)
            putchar(' ');
        puts(argv[i]);
    }
    putchar('\n');
    return 0;
}
