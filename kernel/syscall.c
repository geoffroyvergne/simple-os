#include "syscall.h"
#include "kprintf.h"
#include "keyboard.h"
#include "usermode.h"

/* No per-process address spaces yet, so user pointers are dereferenced
 * directly. Step 8 adds copy_from_user validation with separate page dirs. */

void syscall_dispatch(struct registers *r)
{
    switch (r->eax) {
    case SYS_exit:
        usermode_exit((int)r->ebx);        /* does not return */
        break;

    case SYS_write: {
        int fd = (int)r->ebx;
        const char *buf = (const char *)r->ecx;
        uint32_t len = r->edx;
        if (fd == 1 || fd == 2) {
            for (uint32_t i = 0; i < len; i++)
                kputchar(buf[i]);
            r->eax = len;
        } else {
            r->eax = (uint32_t)-1;
        }
        break;
    }

    case SYS_read: {
        int fd = (int)r->ebx;
        char *buf = (char *)r->ecx;
        uint32_t len = r->edx;
        if (fd == 0) {
            int n = keyboard_readline(buf, len);
            r->eax = (uint32_t)n;
        } else {
            r->eax = (uint32_t)-1;
        }
        break;
    }

    case SYS_getpid:
        r->eax = 1;
        break;

    case SYS_yield:
        r->eax = 0;
        break;

    default:
        kprintf("syscall: unknown number %u\n", r->eax);
        r->eax = (uint32_t)-1;
        break;
    }
}
