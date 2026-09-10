#pragma once
#include "bootinfo.h"

void mm_init(void);
void mm_report(void);

const struct bootinfo *bootinfo_get(void);
