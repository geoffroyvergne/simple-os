#include "libc.h"

static void kv(const char *k, int v, const char *unit)
{
    puts(k);
    for (int s = (int)strlen(k); s < 14; s++)
        putchar(' ');
    putint(v);
    putchar(' ');
    puts(unit);
    putchar('\n');
}

int main(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    struct sysinfo si;
    if (sysinfo(&si) < 0) {
        puts("sysinfo failed\n");
        return 1;
    }
    kv("ram total", (int)si.ram_kb / 1024, "MiB");
    kv("ram free", (int)si.ram_free_kb / 1024, "MiB");
    kv("heap total", (int)si.heap_total_kb, "KiB");
    kv("heap used", (int)si.heap_used_kb, "KiB");
    kv("uptime", (int)si.uptime_ticks / 100, "s");
    return 0;
}
