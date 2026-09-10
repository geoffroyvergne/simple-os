#pragma once
#include <stdint.h>

/* From userprog.asm: a tiny program linked into its own page-aligned .user
 * section and run in ring 3. */
extern void user_entry(void);

/* From usermode.asm. */
int  run_usermode(uint32_t eip, uint32_t esp);   /* returns the user exit code */
void usermode_exit(int code) __attribute__((noreturn));

/* Set up user mappings + TSS esp0 and run the ring-3 demo. */
void usermode_demo(void);
