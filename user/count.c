#include "libc.h"

/* count [label] [n]  -- prints "<label> i" for i = 1..n, ~200 ms apart.
 * Run two of these with '&' to watch the scheduler interleave them. */
static unsigned atou(const char *s)
{
    unsigned v = 0;
    while (*s >= '0' && *s <= '9')
        v = v * 10 + (unsigned)(*s++ - '0');
    return v;
}

int main(int argc, char **argv)
{
    const char *label = argc > 1 ? argv[1] : "count";
    unsigned n = argc > 2 ? atou(argv[2]) : 5;

    for (unsigned i = 1; i <= n; i++) {
        puts(label);
        putchar(' ');
        putint((int)i);
        putchar('\n');
        sleep(200);
    }
    return 0;
}
