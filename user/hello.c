#include "libc.h"

int main(int argc, char **argv)
{
    puts("Hello from an ELF binary running in ring 3!\n");
    puts("  pid  = ");
    putint(getpid());
    putchar('\n');
    puts("  argc = ");
    putint(argc);
    putchar('\n');
    for (int i = 0; i < argc; i++) {
        puts("  argv[");
        putint(i);
        puts("] = ");
        puts(argv[i]);
        putchar('\n');
    }
    return 7;
}
