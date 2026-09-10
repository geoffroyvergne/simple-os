#pragma once
#include <stddef.h>
#include <stdint.h>

void keyboard_init(void);

/* Blocking: waits (hlt) until a byte is available. Returns an ASCII char or a
 * control byte (e.g. 0x03 for Ctrl-C, 0x08 backspace, '\n' for Enter). */
char keyboard_getchar(void);

/* Non-blocking: returns -1 if the buffer is empty. */
int keyboard_trygetchar(void);

/* Read a line with echo and in-line editing (Backspace, Ctrl-U kill line).
 * Stores up to size-1 chars plus a NUL, without the trailing newline.
 * Returns the length, or -1 if the line was cancelled with Ctrl-C. */
int keyboard_readline(char *buf, size_t size);
