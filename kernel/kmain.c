#include "console.h"
#include "kprintf.h"
#include "serial.h"
#include "gdt.h"
#include "tss.h"
#include "interrupts.h"
#include "pit.h"
#include "keyboard.h"
#include "mm.h"
#include "ata.h"
#include "vfs.h"
#include "usermode.h"
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
    kprintf("  --  step 7: user mode + syscalls\n\n");

    gdt_init();
    ok("GDT");
    tss_init();
    ok("TSS loaded");
    interrupts_init();
    ok("IDT + PIC (int 0x80 gate, DPL 3)");
    pit_init(PIT_HZ);
    ok("PIT 100 Hz");

    mm_init();
    ok("PMM + paging + kheap");

    keyboard_init();
    ok("PS/2 keyboard on IRQ1");

    __asm__ volatile("sti");
    ok("interrupts enabled");

    if (ata_init() == 0) {
        vfs_init();
        if (vfs_mounted())
            ok("SimpleFS mounted");
    }

    kprintf("\n");
    usermode_demo();

    kprintf("\n");
    shell_run();
}
