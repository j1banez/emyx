#include <stddef.h>
#include <stdint.h>

#include <kernel/pmm.h>
#include <kernel/syscall.h>
#include <kernel/user.h>
#include <kernel/vmm.h>

#include "gdt.h"

#define USER_SELECTOR_RPL 0x3
#define USER_CODE_SELECTOR (GDT_USER_CODE | USER_SELECTOR_RPL)
#define USER_DATA_SELECTOR (GDT_USER_DATA | USER_SELECTOR_RPL)
#define USER_INIT_STACK_SIZE 4096u
#define PAGE_MASK (~(uintptr_t)(PMM_PAGE_SIZE - 1))

static uint8_t user_init_stack[USER_INIT_STACK_SIZE]
    __attribute__((aligned(PMM_PAGE_SIZE)));

static const char init_message[] = "hello from user init\n";

static void map_user_page(uintptr_t vaddr, uint32_t flags)
{
    uintptr_t page = vaddr & PAGE_MASK;
    uintptr_t paddr;

    if (vmm_get_physaddr(page, &paddr) != 0)
        return;

    vmm_map_page(page, paddr & PAGE_MASK, flags);
}

static uint32_t user_syscall3(uint32_t number, uint32_t arg0,
    uint32_t arg1, uint32_t arg2)
{
    uint32_t ret;

    __asm__ volatile (
        "int $0x80"
        : "=a" (ret)
        : "a" (number), "b" (arg0), "c" (arg1), "d" (arg2)
        : "memory"
    );

    return ret;
}

static void user_init_entry(void)
{
    __asm__ volatile (
        "movw %[data], %%ax\n"
        "movw %%ax, %%ds\n"
        "movw %%ax, %%es\n"
        "movw %%ax, %%fs\n"
        "movw %%ax, %%gs\n"
        :
        : [data] "i" (USER_DATA_SELECTOR)
        : "ax", "memory"
    );

    user_syscall3(SYS_WRITE, SYS_FD_STDOUT, (uint32_t)init_message,
        sizeof(init_message) - 1);
    user_syscall3(SYS_EXIT, 0, 0, 0);

    for (;;)
        ;
}

void user_run_init(void)
{
    uint32_t user_stack = (uint32_t)(user_init_stack + USER_INIT_STACK_SIZE);
    uint32_t eflags;

    /* Temporary: embedded user code still lives in the kernel image. */
    map_user_page((uintptr_t)user_init_entry,
        VMM_PAGE_PRESENT | VMM_PAGE_USER);
    map_user_page((uintptr_t)init_message,
        VMM_PAGE_PRESENT | VMM_PAGE_USER);
    map_user_page((uintptr_t)user_init_stack,
        VMM_PAGE_PRESENT | VMM_PAGE_WRITABLE | VMM_PAGE_USER);

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
          [user_esp] "r" (user_stack),
          [eflags] "r" (eflags),
          [user_cs] "i" (USER_CODE_SELECTOR),
          [user_eip] "r" ((uint32_t)user_init_entry)
        : "memory"
    );

    __builtin_unreachable();
}
