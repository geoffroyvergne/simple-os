#pragma once
#include <stdint.h>

/* Preemptive round-robin multitasking. The kernel itself is not preemptible:
 * a process is only switched out at a trap return to user mode, or when it
 * voluntarily blocks (I/O, wait, sleep, yield). */

/* wait channels */
#define WAIT_NONE   0
#define WAIT_KBD    1
#define WAIT_SLEEP  2
#define WAIT_CHILD  3

void proc_init(void);                    /* the boot thread becomes the idle task */
void proc_run_idle(const char *respawn) __attribute__((noreturn));

int  proc_spawn(const char *path, int argc, char **argv);   /* pid, or < 0 */
int  proc_wait(int pid, int *code);      /* pid reaped, or -1; blocks */
void proc_exit(int code) __attribute__((noreturn));
int  proc_pid(void);
void proc_yield(void);
void proc_sleep(uint32_t ms);

/* Block the current process on a channel, then run the scheduler. */
void proc_block(int chan);
void sched_wake(int chan);               /* make every process on `chan` runnable */

void schedule(void);
void sched_on_tick(void);                /* from the timer IRQ */
int  sched_take_resched(void);           /* read-and-clear the preempt flag */

/* Per-process file descriptors (indices into the VFS open-file table). */
int  proc_fd_alloc(int vfs_handle);
int  proc_fd_get(int fd);
void proc_fd_release(int fd);
