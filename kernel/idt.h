#pragma once
#include <stdint.h>

void idt_init(void);
void idt_set_gate(uint8_t vec, uint32_t handler, uint16_t sel, uint8_t flags);
