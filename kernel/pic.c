#include "pic.h"
#include "io.h"

#define PIC1      0x20
#define PIC2      0xA0
#define PIC1_CMD  PIC1
#define PIC1_DATA (PIC1 + 1)
#define PIC2_CMD  PIC2
#define PIC2_DATA (PIC2 + 1)

#define ICW1_INIT 0x11   /* init + expect ICW4 */
#define ICW4_8086 0x01

static void io_wait(void)
{
    outb(0x80, 0);
}

void pic_remap(void)
{
    uint8_t mask1 = inb(PIC1_DATA);
    uint8_t mask2 = inb(PIC2_DATA);

    outb(PIC1_CMD, ICW1_INIT); io_wait();
    outb(PIC2_CMD, ICW1_INIT); io_wait();
    outb(PIC1_DATA, PIC_IRQ_BASE);       io_wait();  /* master -> 0x20 */
    outb(PIC2_DATA, PIC_IRQ_BASE + 8);   io_wait();  /* slave  -> 0x28 */
    outb(PIC1_DATA, 0x04); io_wait();    /* tell master: slave on IRQ2 */
    outb(PIC2_DATA, 0x02); io_wait();    /* tell slave its cascade identity */
    outb(PIC1_DATA, ICW4_8086); io_wait();
    outb(PIC2_DATA, ICW4_8086); io_wait();

    outb(PIC1_DATA, mask1);
    outb(PIC2_DATA, mask2);
}

void pic_send_eoi(uint8_t irq)
{
    if (irq >= 8)
        outb(PIC2_CMD, 0x20);
    outb(PIC1_CMD, 0x20);
}

void pic_set_mask(uint8_t irq)
{
    uint16_t port = irq < 8 ? PIC1_DATA : PIC2_DATA;
    if (irq >= 8)
        irq -= 8;
    outb(port, inb(port) | (1 << irq));
}

void pic_clear_mask(uint8_t irq)
{
    uint16_t port = irq < 8 ? PIC1_DATA : PIC2_DATA;
    if (irq >= 8)
        irq -= 8;
    outb(port, inb(port) & ~(1 << irq));
}
