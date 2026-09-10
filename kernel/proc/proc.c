#include "proc/proc.h"
#include "mm/vmm.h"
#include "proc/elf.h"
#include "mm/pmm.h"
#include "mm/kheap.h"
#include "lib/string.h"
#include "lib/kprintf.h"
#include "arch/x86/tss.h"

#define KSTACK_SIZE 0x4000       /* 16 KiB per-process kernel stack */
#define MAX_ARGV    16

struct process {
    int       pid;
    uint32_t *dir;
    uint8_t  *kstack;
    char      name[32];
};

static struct process *current;
static int next_pid = 1;

struct process *proc_current(void) { return current; }

void proc_init(void)
{
    current = 0;
}

/* Build argc/argv on the user stack of `dir`. Returns the user esp to start at. */
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

    /* Stack top-down: [strings][NULL][argv[argc-1]]..[argv[0]][argc].
     * crt0 does argv = esp + 4, so argv[] sits inline right above argc. */
    sp -= 4;
    uint32_t zero = 0;
    vmm_copy_to_user(dir, sp, &zero, 4);         /* argv[argc] = NULL */
    for (int i = argc - 1; i >= 0; i--) {
        sp -= 4;
        vmm_copy_to_user(dir, sp, &uargv[i], 4);
    }
    sp -= 4;
    uint32_t argc_u = (uint32_t)argc;
    vmm_copy_to_user(dir, sp, &argc_u, 4);

    return sp;
}

int proc_exec(const char *path, int argc, char **argv)
{
    uint32_t *dir = vmm_create();
    if (!dir)
        return -1;

    uint32_t entry;
    int rc = elf_load(path, dir, &entry);
    if (rc < 0) {
        vmm_destroy(dir);
        return -2;
    }

    /* user stack pages */
    for (uint32_t off = 0; off < USER_STACK_SIZE; off += PAGE_SIZE) {
        uint32_t frame = pmm_alloc_frame();
        memset((void *)frame, 0, PAGE_SIZE);
        vmm_map_user(dir, USER_STACK_TOP - USER_STACK_SIZE + off, frame, 1);
    }
    uint32_t user_esp = setup_stack(dir, argc, argv);

    struct process proc = { .pid = next_pid++, .dir = dir };
    proc.kstack = kmalloc(KSTACK_SIZE);
    strncpy(proc.name, path, sizeof(proc.name) - 1);

    struct process *prev = current;
    current = &proc;
    tss_set_esp0((uint32_t)proc.kstack + KSTACK_SIZE);

    int code = run_user(entry, user_esp, (uint32_t)dir);

    current = prev;
    tss_set_esp0(tss_kstack_top());
    kfree(proc.kstack);
    vmm_destroy(dir);
    return code;
}
