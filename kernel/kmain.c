#include "console.h"
#include "kprintf.h"
#include "serial.h"
#include "gdt.h"
#include "interrupts.h"
#include "pit.h"

static void banner(void)
{
    console_set_color(VGA_LCYAN, VGA_BLACK);
    kprintf("SimpleOS");
    console_set_color(VGA_LGRAY, VGA_BLACK);
    kprintf("  --  step 3: GDT, IDT, PIC, PIT\n\n");
}

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
    banner();

    gdt_init();
    ok("GDT installed (kernel code/data + user selectors)");

    interrupts_init();
    ok("IDT installed (48 gates), 8259 PIC remapped to 0x20/0x28");

    pit_init(PIT_HZ);
    ok("PIT programmed at 100 Hz, IRQ0 unmasked");

    __asm__ volatile("sti");
    ok("interrupts enabled (sti)");

    kprintf("\nCPU exceptions route to panic(); IRQs dispatch via a handler table.\n");
    kprintf("timer heartbeat:\n");

    for (int s = 1; s <= 5; s++) {
        pit_sleep_ms(1000);
        uint32_t t = (uint32_t)pit_ticks();
        kprintf("  %ds elapsed  (ticks=%u)\n", s, t);
    }

    console_set_color(VGA_YELLOW, VGA_BLACK);
    kprintf("\ntimer stable, hlt wakes on IRQ0. next: PS/2 keyboard + line input.\n");

    for (;;)
        __asm__ volatile("hlt");
}
