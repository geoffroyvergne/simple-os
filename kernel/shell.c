#include "shell.h"
#include "keyboard.h"
#include "console.h"
#include "kprintf.h"
#include "string.h"
#include "io.h"
#include "pit.h"
#include "mm.h"
#include "pmm.h"
#include "kheap.h"

#define LINE_MAX 256

static void cmd_help(void)
{
    kprintf("commands:\n");
    kprintf("  help          this text\n");
    kprintf("  echo <text>   print text\n");
    kprintf("  clear         clear the screen\n");
    kprintf("  ticks         PIT tick count since boot\n");
    kprintf("  uptime        seconds since boot\n");
    kprintf("  mem           physical / paging / heap summary\n");
    kprintf("  e820          BIOS memory map\n");
    kprintf("  memtest       exercise kmalloc/kfree\n");
    kprintf("  reboot        reset via the 8042 controller\n");
}

static const char *e820_type(uint32_t t)
{
    switch (t) {
    case 1:  return "usable";
    case 2:  return "reserved";
    case 3:  return "ACPI reclaim";
    case 4:  return "ACPI NVS";
    case 5:  return "bad";
    default: return "?";
    }
}

static void cmd_e820(void)
{
    const struct bootinfo *bi = bootinfo_get();
    if (!bi || bi->magic != BOOTINFO_MAGIC || bi->e820_count == 0) {
        kprintf("no memory map\n");
        return;
    }
    kprintf("  base        length      type\n");
    for (uint32_t i = 0; i < bi->e820_count; i++) {
        const struct e820_entry *e = &bi->e820[i];
        kprintf("  0x%08x  0x%08x  %s\n",
                (uint32_t)e->base, (uint32_t)e->len, e820_type(e->type));
    }
}

static void cmd_memtest(void)
{
    void *a = kmalloc(64);
    void *b = kmalloc(4096);
    void *c = kmalloc(200000);
    kprintf("  kmalloc(64)     -> %p\n", a);
    kprintf("  kmalloc(4096)   -> %p\n", b);
    kprintf("  kmalloc(200000) -> %p\n", c);

    memset(b, 0xAB, 4096);
    kfree(b);
    void *d = kmalloc(2048);
    kprintf("  free(b); kmalloc(2048) -> %p (reuses freed space)\n", d);

    kfree(a);
    kfree(c);
    kfree(d);

    size_t total, used, big;
    kheap_stats(&total, &used, &big);
    kprintf("  after frees: %u KiB used, %u KiB largest free block\n",
            (uint32_t)used / 1024, (uint32_t)big / 1024);
}

static void execute(char *line)
{
    while (*line == ' ')
        line++;

    if (*line == '\0')
        return;

    char *arg = line;
    while (*arg && *arg != ' ')
        arg++;
    int has_arg = (*arg == ' ');
    if (has_arg)
        *arg++ = '\0';
    while (*arg == ' ')
        arg++;

    if (strcmp(line, "help") == 0) {
        cmd_help();
    } else if (strcmp(line, "echo") == 0) {
        kprintf("%s\n", has_arg ? arg : "");
    } else if (strcmp(line, "clear") == 0) {
        console_clear();
    } else if (strcmp(line, "ticks") == 0) {
        kprintf("%u\n", (uint32_t)pit_ticks());
    } else if (strcmp(line, "uptime") == 0) {
        uint32_t t = (uint32_t)pit_ticks();
        kprintf("%u.%02u s\n", t / 100, t % 100);
    } else if (strcmp(line, "mem") == 0) {
        mm_report();
    } else if (strcmp(line, "e820") == 0) {
        cmd_e820();
    } else if (strcmp(line, "memtest") == 0) {
        cmd_memtest();
    } else if (strcmp(line, "reboot") == 0) {
        kprintf("rebooting...\n");
        pit_sleep_ms(200);
        outb(0x64, 0xFE);
    } else {
        kprintf("unknown command: %s\n", line);
    }
}

void shell_run(void)
{
    char line[LINE_MAX];

    kprintf("\ninteractive shell. type 'help'.\n\n");

    for (;;) {
        console_set_color(VGA_LCYAN, VGA_BLACK);
        kprintf("simple-os> ");
        console_set_color(VGA_LGRAY, VGA_BLACK);

        int n = keyboard_readline(line, sizeof(line));
        if (n < 0) {
            kprintf("^C\n");
            continue;
        }
        execute(line);
    }
}
