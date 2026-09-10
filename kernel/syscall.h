#pragma once
#include "interrupts.h"

/* Syscall ABI: eax = number, ebx/ecx/edx = args, result in eax.
 * Invoked from ring 3 with `int 0x80`. */

#define SYS_exit   0
#define SYS_write  1
#define SYS_read   2
#define SYS_getpid 3
#define SYS_yield  4

void syscall_dispatch(struct registers *r);
