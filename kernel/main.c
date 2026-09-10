#include "term/console.h"
#include "lib/kprintf.h"
#include "drivers/serial.h"
#include "arch/x86/gdt.h"
#include "arch/x86/tss.h"
#include "arch/x86/interrupts.h"
#include "drivers/pit.h"
#include "drivers/keyboard.h"
#include "mm/mm.h"
#include "drivers/ata.h"
#include "fs/vfs.h"
#include "proc/proc.h"
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
    kprintf("  --  step 8: ELF loader + processes\n\n");

    gdt_init();
    ok("GDT");
    tss_init();
    ok("TSS");
    interrupts_init();
    ok("IDT + PIC + syscall gate");
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

    proc_init();
    ok("process subsystem");

    kprintf("\n");
    shell_run();
}
