#include <stdint.h>

#include <kernel/user.h>

#include "gdt.h"

#define USER_SELECTOR_RPL 0x3
#define USER_CODE_SELECTOR (GDT_USER_CODE | USER_SELECTOR_RPL)
#define USER_DATA_SELECTOR (GDT_USER_DATA | USER_SELECTOR_RPL)

void user_enter(user_process *process)
{
    uint32_t eflags;
    uintptr_t kernel_stack;

    if (process == NULL)
        return;

    __asm__ volatile ("mov %%esp, %0" : "=r" (kernel_stack));
    tss_set_kernel_stack(kernel_stack);

    __asm__ volatile ("pushf; pop %0" : "=r" (eflags));
    eflags |= 0x200;

    __asm__ volatile (
        "pushl %[user_ss]\n"
        "pushl %[user_esp]\n"
        "pushl %[eflags]\n"
        "pushl %[user_cs]\n"
        "pushl %[user_eip]\n"
        "iret\n"
        :
        : [user_ss] "i" (USER_DATA_SELECTOR),
          [user_esp] "r" (process->stack_top),
          [eflags] "r" (eflags),
          [user_cs] "i" (USER_CODE_SELECTOR),
          [user_eip] "r" (process->entry)
        : "memory"
    );

    __builtin_unreachable();
}
