#include "console.h"
#include "kprintf.h"
#include "serial.h"
#include "gdt.h"
#include "interrupts.h"
#include "pit.h"
#include "keyboard.h"
#include "mm.h"
#include "shell.h"

static void ok(const char *what)
{
    console_set_color(VGA_LGREEN, VGA_BLACK);
    kprintf("[ok] ");
    console_set_color(VGA_LGRAY, VGA_BLACK);
    kprintf("%s\n", what);
}

void kmain(void)
{
    serial_init();
    console_init();

    console_set_color(VGA_LCYAN, VGA_BLACK);
    kprintf("SimpleOS");
    console_set_color(VGA_LGRAY, VGA_BLACK);
    kprintf("  --  step 5: memory management\n\n");

    gdt_init();
    ok("GDT");
    interrupts_init();
    ok("IDT + PIC");
    pit_init(PIT_HZ);
    ok("PIT 100 Hz");

    mm_init();
    ok("PMM + paging + kheap");

    keyboard_init();
    ok("PS/2 keyboard on IRQ1");

    __asm__ volatile("sti");
    ok("interrupts enabled");

    kprintf("\n");
    mm_report();

    shell_run();
}
