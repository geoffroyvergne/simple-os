#pragma once
#include <stdint.h>

#define PIT_HZ 100

void     pit_init(uint32_t hz);
uint64_t pit_ticks(void);
void     pit_sleep_ms(uint32_t ms);
