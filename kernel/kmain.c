#include "console.h"
#include "kprintf.h"
#include "serial.h"

static void banner(void)
{
    console_set_color(VGA_LCYAN, VGA_BLACK);
    kprintf("SimpleOS");
    console_set_color(VGA_LGRAY, VGA_BLACK);
    kprintf("  --  step 2: scrolling terminal + kprintf\n\n");
}

void kmain(void)
{
    serial_init();
    console_init();

    banner();

    console_set_color(VGA_LGREEN, VGA_BLACK);
    kprintf("[ok] serial COM1 up\n");
    kprintf("[ok] VGA text console up (80x25)\n\n");
    console_set_color(VGA_LGRAY, VGA_BLACK);

    kprintf("kprintf self-test:\n");
    kprintf("  char     : %c%c%c\n", 'a', 'b', 'c');
    kprintf("  string   : %s / %8s|\n", "hello", "pad");
    kprintf("  signed   : %d %d %d\n", 0, -42, 2147483647);
    kprintf("  unsigned : %u\n", 4000000000u);
    kprintf("  hex      : %x %X %08x\n", 0xdead, 0xbeef, 0x1234);
    kprintf("  binary   : %b\n", 0xA5u);
    kprintf("  pointer  : %p\n", (void *)&kmain);
    kprintf("  percent  : 100%%\n\n");

    kprintf("scroll test (40 lines into a 25-line screen):\n");
    for (int i = 1; i <= 40; i++)
        kprintf("  line %2d\n", i);

    console_set_color(VGA_YELLOW, VGA_BLACK);
    kprintf("\nhalted. next: GDT reload, IDT, PIC remap, timer, keyboard.\n");

    for (;;)
        __asm__ volatile("hlt");
}
