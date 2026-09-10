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
#include "vfs.h"
#include "sfs.h"

#define LINE_MAX 256

static void cmd_help(void)
{
    kprintf("commands:\n");
    kprintf("  help            this text\n");
    kprintf("  echo <text>     print text\n");
    kprintf("  clear           clear the screen\n");
    kprintf("  uptime / ticks  time since boot\n");
    kprintf("  mem / e820      memory summary / BIOS map\n");
    kprintf("  memtest         exercise kmalloc/kfree\n");
    kprintf("  ls              list files\n");
    kprintf("  cat <file>      print a file\n");
    kprintf("  hexdump <file>  hex + ASCII dump\n");
    kprintf("  stat <file>     file size / capacity\n");
    kprintf("  df              filesystem usage\n");
    kprintf("  touch <file>    create an empty file\n");
    kprintf("  write <file> <text>   append a line to a file\n");
    kprintf("  rm <file>       delete a file\n");
    kprintf("  reboot          reset via the 8042 controller\n");
}

static void cmd_ls(void)
{
    struct vfs_dirent d;
    int n = 0;
    for (int i = 0; vfs_readdir(i, &d) == 0; i++) {
        kprintf("  %s", d.name);
        for (int s = (int)strlen(d.name); s < 16; s++)
            kputchar(' ');
        kprintf("%u bytes\n", d.size);
        n++;
    }
    if (n == 0)
        kprintf("  (empty)\n");
}

static void cmd_cat(const char *name)
{
    int fd = vfs_open(name);
    if (fd < 0) {
        kprintf("cat: %s: not found\n", name);
        return;
    }
    char buf[256];
    int r;
    while ((r = vfs_read(fd, buf, sizeof(buf))) > 0)
        for (int i = 0; i < r; i++)
            kputchar(buf[i]);
    vfs_close(fd);
}

static void cmd_hexdump(const char *name)
{
    int fd = vfs_open(name);
    if (fd < 0) {
        kprintf("hexdump: %s: not found\n", name);
        return;
    }
    uint8_t buf[16];
    uint32_t off = 0;
    int r;
    while ((r = vfs_read(fd, buf, sizeof(buf))) > 0) {
        kprintf("%08x  ", off);
        for (int i = 0; i < 16; i++) {
            if (i < r)
                kprintf("%02x ", buf[i]);
            else
                kprintf("   ");
        }
        kprintf(" |");
        for (int i = 0; i < r; i++)
            kputchar(buf[i] >= 0x20 && buf[i] < 0x7F ? buf[i] : '.');
        kprintf("|\n");
        off += (uint32_t)r;
    }
    vfs_close(fd);
}

static void cmd_stat(const char *name)
{
    struct vfs_stat st;
    if (vfs_stat(name, &st) < 0) {
        kprintf("stat: %s: not found\n", name);
        return;
    }
    kprintf("  %s: %u bytes, %u bytes capacity\n", name, st.size, st.capacity);
}

static void cmd_df(void)
{
    if (!vfs_mounted()) {
        kprintf("no filesystem\n");
        return;
    }
    uint32_t total, used, files;
    sfs_statfs(&total, &used, &files);
    kprintf("  SimpleFS: %u/%u blocks used (%u KiB / %u KiB), %u file(s)\n",
            used, total, used / 2, total / 2, files);
}

static void cmd_write(char *name)
{
    char *text = name;
    while (*text && *text != ' ')
        text++;
    if (*text == ' ')
        *text++ = '\0';

    if (vfs_stat(name, 0) < 0 && vfs_create(name, 4096) < 0) {
        kprintf("write: cannot create %s\n", name);
        return;
    }
    int fd = vfs_open(name);
    if (fd < 0) {
        kprintf("write: cannot open %s\n", name);
        return;
    }
    struct vfs_stat st;
    vfs_stat(name, &st);
    vfs_seek(fd, st.size);
    vfs_write(fd, text, (uint32_t)strlen(text));
    vfs_write(fd, "\n", 1);
    vfs_close(fd);
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
    } else if (strcmp(line, "ls") == 0) {
        cmd_ls();
    } else if (strcmp(line, "cat") == 0 && has_arg) {
        cmd_cat(arg);
    } else if (strcmp(line, "hexdump") == 0 && has_arg) {
        cmd_hexdump(arg);
    } else if (strcmp(line, "stat") == 0 && has_arg) {
        cmd_stat(arg);
    } else if (strcmp(line, "df") == 0) {
        cmd_df();
    } else if (strcmp(line, "touch") == 0 && has_arg) {
        if (vfs_create(arg, 4096) < 0)
            kprintf("touch: cannot create %s\n", arg);
    } else if (strcmp(line, "write") == 0 && has_arg) {
        cmd_write(arg);
    } else if (strcmp(line, "rm") == 0 && has_arg) {
        if (vfs_unlink(arg) < 0)
            kprintf("rm: %s: not found\n", arg);
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
