#include "syscall.h"
#include "kprintf.h"
#include "keyboard.h"
#include "proc.h"
#include "vmm.h"

/* User pointers now belong to a separate (but currently co-mapped) address
 * space. Bounds-check them against the user region; full page-walk validation
 * can come with demand paging. */
static int user_range_ok(uint32_t addr, uint32_t len)
{
    if (len > USER_STACK_TOP)
        return 0;
    return addr >= USER_BASE && addr + len <= USER_STACK_TOP;
}

void syscall_dispatch(struct registers *r)
{
    switch (r->eax) {
    case SYS_exit:
        user_exit((int)r->ebx);            /* does not return */
        break;

    case SYS_write: {
        int fd = (int)r->ebx;
        const char *buf = (const char *)r->ecx;
        uint32_t len = r->edx;
        if ((fd == 1 || fd == 2) && user_range_ok(r->ecx, len)) {
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
        if (fd == 0 && user_range_ok(r->ecx, len)) {
            r->eax = (uint32_t)keyboard_readline(buf, len);
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
