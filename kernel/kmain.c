#include "vga.h"
#include "serial.h"

void kmain(void)
{
    serial_init();
    serial_write("\n[SimpleOS] kmain reached: 32-bit protected mode, custom bootloader OK\n");

    uint8_t normal = vga_attr(VGA_LGRAY, VGA_BLACK);
    vga_clear(normal);
    vga_puts_at(0, 0, "SimpleOS", vga_attr(VGA_WHITE, VGA_BLACK));
    vga_puts_at(1, 0, "Step 1: custom bootloader + 32-bit C kernel booted.",
                vga_attr(VGA_LGREEN, VGA_BLACK));
    vga_puts_at(3, 0, "Next: scrolling terminal, IDT/interrupts, keyboard.", normal);
    vga_puts_at(5, 0, "System halted.", vga_attr(VGA_YELLOW, VGA_BLACK));

    serial_write("[SimpleOS] halted.\n");
    for (;;)
        __asm__ volatile("hlt");
}
