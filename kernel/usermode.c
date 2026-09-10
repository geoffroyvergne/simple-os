#include "usermode.h"
#include "paging.h"
#include "tss.h"
#include "kprintf.h"

extern uint8_t __user_start[], __user_end[];

static uint8_t user_stack[8192] __attribute__((aligned(4096)));

void usermode_demo(void)
{
    paging_set_user((uint32_t)__user_start,
                    (uint32_t)(__user_end - __user_start));
    paging_set_user((uint32_t)user_stack, sizeof(user_stack));

    tss_set_esp0(tss_kstack_top());

    kprintf("usermode: entering ring 3 at %p\n", (void *)user_entry);
    int code = run_usermode((uint32_t)user_entry,
                            (uint32_t)(user_stack + sizeof(user_stack)));
    kprintf("usermode: user process exited, code=%d\n", code);
}
