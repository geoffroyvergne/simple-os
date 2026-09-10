#pragma once
#include <stdint.h>

/* Single-process, synchronous exec: the shell (in the kernel) calls
 * proc_exec(), which builds an address space, loads the ELF, drops to ring 3,
 * and returns when the program calls SYS_exit. A real scheduler / concurrent
 * processes come in a later step. */

void proc_init(void);

/* Run an ELF from the filesystem. Returns the program's exit code, or a
 * negative value if it could not be started. */
int proc_exec(const char *path, int argc, char **argv);

/* From ring3.asm. */
int  run_user(uint32_t entry, uint32_t user_esp, uint32_t cr3);
void user_exit(int code) __attribute__((noreturn));
