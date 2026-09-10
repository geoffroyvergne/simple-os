#pragma once
#include "arch/x86/interrupts.h"

/* Syscall ABI: eax = number, ebx/ecx/edx = args, result in eax.
 * Invoked from ring 3 with `int 0x80`. */

#define SYS_exit     0
#define SYS_write    1     /* (fd, buf, len)  fd 1/2 -> console, fd >= 3 -> file */
#define SYS_read     2     /* (fd, buf, len)  fd 0 -> keyboard line, fd >= 3 -> file */
#define SYS_getpid   3
#define SYS_yield    4
#define SYS_open     5     /* (path, flags)  flags: 0 = read, 1 = write/create */
#define SYS_close    6     /* (fd) */
#define SYS_lseek    7     /* (fd, offset)   absolute */
#define SYS_readdir  8     /* (index, struct sys_dirent *) */
#define SYS_stat     9     /* (path, struct sys_stat *) */
#define SYS_spawn    10    /* (path, char **argv)  blocks; returns child exit code */
#define SYS_unlink   11    /* (path) */
#define SYS_sysinfo  12    /* (struct sys_info *) */
#define SYS_reboot   13

#define O_RDONLY 0
#define O_WRCREAT 1

struct sys_dirent {
    char     name[40];
    uint32_t size;
};

struct sys_stat {
    uint32_t size;
    uint32_t capacity;
};

struct sys_info {
    uint32_t ram_kb;
    uint32_t ram_free_kb;
    uint32_t heap_total_kb;
    uint32_t heap_used_kb;
    uint32_t uptime_ticks;
};

void syscall_dispatch(struct registers *r);
