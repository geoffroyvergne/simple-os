#pragma once
#include "arch/x86/bootinfo.h"

void mm_init(void);
void mm_report(void);

const struct bootinfo *bootinfo_get(void);
