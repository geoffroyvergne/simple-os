#pragma once

/* A tiny line-oriented shell. Expands into a real user-mode program in a
 * later step; for now it runs in the kernel as a driver for keyboard input. */
__attribute__((noreturn)) void shell_run(void);
