#pragma once
#include <stdint.h>

/* One Task State Segment, used only so ring 3 -> ring 0 transitions
 * (int 0x80, IRQs, exceptions) load a known-good kernel stack. */

void     tss_init(void);
void     tss_set_esp0(uint32_t esp0);
uint32_t tss_kstack_top(void);
