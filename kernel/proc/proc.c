#include "proc/proc.h"
#include "proc/elf.h"
#include "mm/vmm.h"
#include "mm/pmm.h"
#include "mm/kheap.h"
#include "lib/string.h"
#include "lib/kprintf.h"
#include "arch/x86/tss.h"
#include "arch/x86/paging.h"
#include "arch/x86/irqflags.h"
#include "drivers/pit.h"

#define MAXPROC      16
#define KSTACK_SIZE  0x4000        /* 16 KiB */
#define MAX_ARGV     16
#define QUANTUM      5             /* timer ticks per slice (50 ms) */
#define NFD          8

enum pstate { P_UNUSED, P_RUNNABLE, P_RUNNING, P_BLOCKED, P_ZOMBIE };

struct process {
    int              pid;
    enum pstate      state;
    uint32_t        *dir;
    uint32_t         cr3;
    uint8_t         *kstack;
    uint32_t         kstack_top;
    uint32_t         ksp;             /* saved kernel esp while not running */
    uint32_t         entry, user_esp;
    int              exit_code;
    int              wait_chan;
    uint64_t         wake_tick;
    struct process  *parent;
    char             name[24];
    int              fds[NFD];        /* -> VFS handle, -1 = free */
};

extern void switch_context(uint32_t *save_ksp, uint32_t load_ksp, uint32_t load_cr3);
extern void enter_user(uint32_t entry, uint32_t user_esp) __attribute__((noreturn));

static struct process procs[MAXPROC];
static struct process *cur;
static int next_pid = 1;
static int rr_cursor;
static volatile int need_resched;
static int quantum_left = QUANTUM;

/* ---- scheduler core ---- */

static struct process *pick_next(void)
{
    for (int n = 0; n < MAXPROC - 1; n++) {
        rr_cursor = rr_cursor % (MAXPROC - 1) + 1;   /* 1 .. MAXPROC-1 */
        if (procs[rr_cursor].state == P_RUNNABLE)
            return &procs[rr_cursor];
    }
    return &procs[0];                                /* idle */
}

void schedule(void)
{
    uint32_t f = irq_save();
    struct process *prev = cur;
    struct process *next = pick_next();

    if (next != prev) {
        if (prev->state == P_RUNNING)
            prev->state = P_RUNNABLE;
        next->state = P_RUNNING;
        cur = next;
        tss_set_esp0(next->kstack_top);
        switch_context(&prev->ksp, next->ksp, next->cr3);
        /* resumed later: `cur` is `prev` again */
    }
    irq_restore(f);
}

void proc_block(int chan)
{
    cur->state = P_BLOCKED;
    cur->wait_chan = chan;
}

void sched_wake(int chan)
{
    for (int i = 1; i < MAXPROC; i++) {
        if (procs[i].state == P_BLOCKED && procs[i].wait_chan == chan) {
            procs[i].state = P_RUNNABLE;
            procs[i].wait_chan = WAIT_NONE;
        }
    }
}

static void wake_sleepers(void)
{
    uint64_t now = pit_ticks();
    for (int i = 1; i < MAXPROC; i++) {
        if (procs[i].state == P_BLOCKED && procs[i].wait_chan == WAIT_SLEEP &&
            now >= procs[i].wake_tick) {
            procs[i].state = P_RUNNABLE;
            procs[i].wait_chan = WAIT_NONE;
        }
    }
}

void sched_on_tick(void)
{
    wake_sleepers();
    if (--quantum_left <= 0) {
        quantum_left = QUANTUM;
        need_resched = 1;
    }
}

int sched_take_resched(void)
{
    int r = need_resched;
    need_resched = 0;
    return r;
}

void proc_yield(void)
{
    schedule();
}

void proc_sleep(uint32_t ms)
{
    uint32_t f = irq_save();
    cur->wake_tick = pit_ticks() + (ms * PIT_HZ) / 1000 + 1;
    proc_block(WAIT_SLEEP);
    irq_restore(f);
    schedule();
}

/* ---- process lifecycle ---- */

int proc_pid(void) { return cur ? cur->pid : 0; }

static struct process *alloc_proc(void)
{
    for (int i = 1; i < MAXPROC; i++)
        if (procs[i].state == P_UNUSED)
            return &procs[i];
    return 0;
}

static void reap(struct process *p)
{
    kfree(p->kstack);
    vmm_destroy(p->dir);
    memset(p, 0, sizeof(*p));
    p->state = P_UNUSED;
}

void proc_init(void)
{
    memset(procs, 0, sizeof(procs));
    cur = &procs[0];
    cur->pid = 0;
    cur->state = P_RUNNING;
    cur->dir = paging_kernel_dir();
    cur->cr3 = paging_kernel_dir_phys();
    cur->kstack_top = tss_kstack_top();
    strncpy(cur->name, "idle", sizeof(cur->name) - 1);
}

static uint32_t setup_stack(uint32_t *dir, int argc, char **argv)
{
    uint32_t uargv[MAX_ARGV];
    uint32_t sp = USER_STACK_TOP;

    if (argc > MAX_ARGV)
        argc = MAX_ARGV;

    for (int i = argc - 1; i >= 0; i--) {
        uint32_t len = (uint32_t)strlen(argv[i]) + 1;
        sp -= len;
        sp &= ~3u;
        vmm_copy_to_user(dir, sp, argv[i], len);
        uargv[i] = sp;
    }
    sp -= 4;
    uint32_t zero = 0;
    vmm_copy_to_user(dir, sp, &zero, 4);
    for (int i = argc - 1; i >= 0; i--) {
        sp -= 4;
        vmm_copy_to_user(dir, sp, &uargv[i], 4);
    }
    sp -= 4;
    uint32_t argc_u = (uint32_t)argc;
    vmm_copy_to_user(dir, sp, &argc_u, 4);
    return sp;
}

/* First thing a brand-new process runs, on its own kernel stack. */
static void proc_bootstrap(void)
{
    tss_set_esp0(cur->kstack_top);
    enter_user(cur->entry, cur->user_esp);
}

int proc_spawn(const char *path, int argc, char **argv)
{
    struct process *p = alloc_proc();
    if (!p)
        return -1;

    uint32_t *dir = vmm_create();
    if (!dir)
        return -1;

    uint32_t entry;
    if (elf_load(path, dir, &entry) < 0) {
        vmm_destroy(dir);
        return -2;
    }

    for (uint32_t off = 0; off < USER_STACK_SIZE; off += PAGE_SIZE) {
        uint32_t frame = pmm_alloc_frame();
        memset((void *)frame, 0, PAGE_SIZE);
        vmm_map_user(dir, USER_STACK_TOP - USER_STACK_SIZE + off, frame, 1);
    }

    p->dir = dir;
    p->cr3 = (uint32_t)dir;
    p->entry = entry;
    p->user_esp = setup_stack(dir, argc, argv);
    p->kstack = kmalloc(KSTACK_SIZE);
    p->kstack_top = (uint32_t)p->kstack + KSTACK_SIZE;
    p->exit_code = 0;
    p->wait_chan = WAIT_NONE;
    p->parent = cur;
    strncpy(p->name, path, sizeof(p->name) - 1);
    for (int i = 0; i < NFD; i++)
        p->fds[i] = -1;

    /* Craft the kernel stack so the first switch_context "returns" into
     * proc_bootstrap: [ebp][edi][esi][ebx][ret]. */
    uint32_t *sp = (uint32_t *)(p->kstack + KSTACK_SIZE);
    *(--sp) = (uint32_t)proc_bootstrap;
    *(--sp) = 0;                       /* ebx */
    *(--sp) = 0;                       /* esi */
    *(--sp) = 0;                       /* edi */
    *(--sp) = 0;                       /* ebp */
    p->ksp = (uint32_t)sp;

    p->pid = next_pid++;

    uint32_t f = irq_save();
    p->state = P_RUNNABLE;
    irq_restore(f);
    return p->pid;
}

void proc_exit(int code)
{
    (void)irq_save();                  /* stay off until we switch away */
    cur->exit_code = code;
    cur->state = P_ZOMBIE;

    for (int i = 1; i < MAXPROC; i++)
        if (procs[i].state != P_UNUSED && procs[i].parent == cur)
            procs[i].parent = &procs[0];

    if (cur->parent && cur->parent->state == P_BLOCKED &&
        cur->parent->wait_chan == WAIT_CHILD)
        cur->parent->state = P_RUNNABLE;

    schedule();                        /* never returns */
    for (;;)
        ;
}

int proc_wait(int pid, int *code)
{
    int nonblock = (pid == 0);       /* pid 0 = poll; pid < 0 = any child */

    for (;;) {
        uint32_t f = irq_save();
        int have_child = 0;

        for (int i = 1; i < MAXPROC; i++) {
            struct process *p = &procs[i];
            if (p->state == P_UNUSED || p->parent != cur)
                continue;
            if (pid > 0 && p->pid != pid)
                continue;
            have_child = 1;
            if (p->state == P_ZOMBIE) {
                int rc = p->exit_code, rpid = p->pid;
                reap(p);
                irq_restore(f);
                if (code)
                    *code = rc;
                return rpid;
            }
        }
        if (!have_child || nonblock) {
            irq_restore(f);
            return -1;
        }
        proc_block(WAIT_CHILD);
        irq_restore(f);
        schedule();
    }
}

void proc_run_idle(const char *respawn)
{
    for (;;) {
        for (int i = 1; i < MAXPROC; i++)
            if (procs[i].state == P_ZOMBIE && procs[i].parent == &procs[0])
                reap(&procs[i]);

        int alive = 0;
        for (int i = 1; i < MAXPROC; i++)
            if (procs[i].state != P_UNUSED)
                alive = 1;
        if (!alive && respawn) {
            char *av[] = { (char *)respawn, 0 };
            proc_spawn(respawn, 1, av);
        }

        __asm__ volatile("sti; hlt");
        schedule();
    }
}

/* ---- per-process fds ---- */

int proc_fd_alloc(int vfs_handle)
{
    for (int i = 0; i < NFD; i++) {
        if (cur->fds[i] < 0) {
            cur->fds[i] = vfs_handle;
            return i + 3;
        }
    }
    return -1;
}

int proc_fd_get(int fd)
{
    fd -= 3;
    if (fd < 0 || fd >= NFD)
        return -1;
    return cur->fds[fd];
}

void proc_fd_release(int fd)
{
    fd -= 3;
    if (fd >= 0 && fd < NFD)
        cur->fds[fd] = -1;
}
