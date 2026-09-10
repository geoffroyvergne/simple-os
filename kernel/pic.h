#pragma once
#include <stdint.h>

#define PIC_IRQ_BASE 32   /* IRQ0..15 are remapped to vectors 32..47 */

void pic_remap(void);
void pic_send_eoi(uint8_t irq);
void pic_set_mask(uint8_t irq);
void pic_clear_mask(uint8_t irq);
