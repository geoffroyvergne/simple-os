#include "proc/syscall.h"
#include "proc/proc.h"
#include "lib/kprintf.h"
#include "lib/string.h"
#include "drivers/keyboard.h"
#include "mm/vmm.h"
#include "mm/pmm.h"
#include "mm/kheap.h"
#include "fs/vfs.h"
#include "drivers/pit.h"
#include "arch/x86/io.h"

#define MAX_ARGV  16
#define FD_BASE   3       /* user fds 0/1/2 are the console/keyboard */

/* ---- user memory access (caller's address space is still current) ---- */

static int urange(uint32_t addr, uint32_t n)
{
    if (n > USER_STACK_TOP)
        return 0;
    return addr >= USER_BASE && addr + n <= USER_STACK_TOP;
}

static int ucopy_in(void *dst, uint32_t uaddr, uint32_t n)
{
    if (!urange(uaddr, n))
        return -1;
    memcpy(dst, (const void *)uaddr, n);
    return 0;
}

static int ucopy_out(uint32_t uaddr, const void *src, uint32_t n)
{
    if (!urange(uaddr, n))
        return -1;
    memcpy((void *)uaddr, src, n);
    return 0;
}

/* Copy a NUL-terminated string in; returns length, or -1 if unmapped/too long. */
static int ustr(char *dst, uint32_t uaddr, uint32_t max)
{
    for (uint32_t i = 0; i < max; i++) {
        if (!urange(uaddr + i, 1))
            return -1;
        char c = *(const char *)(uaddr + i);
        dst[i] = c;
        if (!c)
            return (int)i;
    }
    return -1;
}

/* ---- individual calls ---- */

static int sys_write(int fd, uint32_t ubuf, uint32_t len)
{
    if (fd == 1 || fd == 2) {
        if (!urange(ubuf, len))
            return -1;
        const char *p = (const char *)ubuf;
        for (uint32_t i = 0; i < len; i++)
            kputchar(p[i]);
        return (int)len;
    }
    if (fd >= FD_BASE) {
        if (!urange(ubuf, len))
            return -1;
        return vfs_write(fd - FD_BASE, (const void *)ubuf, len);
    }
    return -1;
}

static int sys_read(int fd, uint32_t ubuf, uint32_t len)
{
    if (!urange(ubuf, len))
        return -1;
    if (fd == 0)
        return keyboard_readline((char *)ubuf, len);
    if (fd >= FD_BASE)
        return vfs_read(fd - FD_BASE, (void *)ubuf, len);
    return -1;
}

static int sys_open(uint32_t upath, int flags)
{
    char path[64];
    if (ustr(path, upath, sizeof(path)) < 0)
        return -1;

    if (flags == O_WRCREAT) {
        struct vfs_stat st;
        if (vfs_stat(path, &st) < 0 && vfs_create(path, 64 * 1024) < 0)
            return -1;
    }
    int fd = vfs_open(path);
    return fd < 0 ? -1 : fd + FD_BASE;
}

static int sys_readdir(int index, uint32_t uout)
{
    struct vfs_dirent d;
    if (vfs_readdir(index, &d) < 0)
        return -1;
    struct sys_dirent out;
    memcpy(out.name, d.name, sizeof(out.name));
    out.name[sizeof(out.name) - 1] = '\0';
    out.size = d.size;
    return ucopy_out(uout, &out, sizeof(out));
}

static int sys_stat(uint32_t upath, uint32_t uout)
{
    char path[64];
    if (ustr(path, upath, sizeof(path)) < 0)
        return -1;
    struct vfs_stat vs;
    if (vfs_stat(path, &vs) < 0)
        return -1;
    struct sys_stat out = { .size = vs.size, .capacity = vs.capacity };
    return ucopy_out(uout, &out, sizeof(out));
}

static int sys_unlink(uint32_t upath)
{
    char path[64];
    if (ustr(path, upath, sizeof(path)) < 0)
        return -1;
    return vfs_unlink(path);
}

static int sys_spawn(uint32_t upath, uint32_t uargv)
{
    char path[64];
    if (ustr(path, upath, sizeof(path)) < 0)
        return -1;

    char *kargv[MAX_ARGV];
    static char kbuf[MAX_ARGV * 64];
    int argc = 0;
    uint32_t off = 0;

    if (uargv) {
        for (; argc < MAX_ARGV - 1; argc++) {
            uint32_t p;
            if (ucopy_in(&p, uargv + (uint32_t)argc * 4, 4) < 0)
                return -1;
            if (p == 0)
                break;
            int len = ustr(kbuf + off, p, sizeof(kbuf) - off);
            if (len < 0)
                return -1;
            kargv[argc] = kbuf + off;
            off += (uint32_t)len + 1;
        }
    }
    if (argc == 0) {
        kargv[0] = path;
        argc = 1;
    }
    return proc_exec(path, argc, kargv);
}

static int sys_sysinfo(uint32_t uout)
{
    size_t ht, hu, hb;
    kheap_stats(&ht, &hu, &hb);
    struct sys_info info = {
        .ram_kb        = pmm_total_frames() * 4,
        .ram_free_kb   = pmm_free_frames() * 4,
        .heap_total_kb = (uint32_t)ht / 1024,
        .heap_used_kb  = (uint32_t)hu / 1024,
        .uptime_ticks  = (uint32_t)pit_ticks(),
    };
    return ucopy_out(uout, &info, sizeof(info));
}

void syscall_dispatch(struct registers *r)
{
    switch (r->eax) {
    case SYS_exit:
        user_exit((int)r->ebx);              /* does not return */
        break;
    case SYS_write:
        r->eax = (uint32_t)sys_write((int)r->ebx, r->ecx, r->edx);
        break;
    case SYS_read:
        r->eax = (uint32_t)sys_read((int)r->ebx, r->ecx, r->edx);
        break;
    case SYS_getpid:
        r->eax = (uint32_t)proc_pid();
        break;
    case SYS_yield:
        r->eax = 0;
        break;
    case SYS_open:
        r->eax = (uint32_t)sys_open(r->ebx, (int)r->ecx);
        break;
    case SYS_close:
        if ((int)r->ebx >= FD_BASE)
            vfs_close((int)r->ebx - FD_BASE);
        r->eax = 0;
        break;
    case SYS_lseek:
        r->eax = (int)r->ebx >= FD_BASE && vfs_seek((int)r->ebx - FD_BASE, r->ecx) == 0
                     ? r->ecx : (uint32_t)-1;
        break;
    case SYS_readdir:
        r->eax = (uint32_t)sys_readdir((int)r->ebx, r->ecx);
        break;
    case SYS_stat:
        r->eax = (uint32_t)sys_stat(r->ebx, r->ecx);
        break;
    case SYS_spawn:
        r->eax = (uint32_t)sys_spawn(r->ebx, r->ecx);
        break;
    case SYS_unlink:
        r->eax = (uint32_t)sys_unlink(r->ebx);
        break;
    case SYS_sysinfo:
        r->eax = (uint32_t)sys_sysinfo(r->ebx);
        break;
    case SYS_reboot:
        outb(0x64, 0xFE);
        break;
    default:
        kprintf("syscall: unknown number %u\n", r->eax);
        r->eax = (uint32_t)-1;
        break;
    }
}
