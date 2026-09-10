#include "drivers/pit.h"
#include "arch/x86/io.h"
#include "arch/x86/interrupts.h"
#include "arch/x86/pic.h"
#include "proc/proc.h"

#define PIT_CH0   0x40
#define PIT_CMD   0x43
#define PIT_BASE  1193182u

static volatile uint64_t ticks;
static uint32_t freq_hz = PIT_HZ;

static void on_tick(struct registers *r)
{
    (void)r;
    ticks++;
    sched_on_tick();
}

uint64_t pit_ticks(void)
{
    return ticks;
}

void pit_sleep_ms(uint32_t ms)
{
    /* 32-bit math only: i686 has no hardware 64-bit divide. */
    uint32_t delta = (ms / 1000) * freq_hz + (ms % 1000) * freq_hz / 1000;
    if (delta == 0)
        delta = 1;
    uint64_t target = ticks + delta;
    while (ticks < target)
        __asm__ volatile("hlt");
}

void pit_init(uint32_t hz)
{
    freq_hz = hz;
    uint32_t divisor = PIT_BASE / hz;

    outb(PIT_CMD, 0x36);                       /* ch0, lobyte/hibyte, mode 3 */
    outb(PIT_CH0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CH0, (uint8_t)((divisor >> 8) & 0xFF));

    irq_install_handler(0, on_tick);
    pic_clear_mask(0);
}
