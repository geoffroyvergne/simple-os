#pragma once
#include <stdint.h>

/* Processes run one at a time: a running process may spawn another, which
 * suspends the parent until the child exits (nested, synchronous exec). A
 * preemptive scheduler is a later step. */

void proc_init(void);

/* Load and run an ELF from the filesystem. Returns the program's exit code,
 * or negative if it could not be started (-2 = not found / not an ELF). */
int proc_exec(const char *path, int argc, char **argv);

int proc_pid(void);

/* From ring3.asm. */
int  run_user(uint32_t entry, uint32_t user_esp, uint32_t cr3);
void user_exit(int code) __attribute__((noreturn));
