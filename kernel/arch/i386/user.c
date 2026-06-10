#include <stdint.h>

#include <kernel/pmm.h>
#include <kernel/printk.h>
#include <kernel/user.h>
#include <kernel/vmm.h>

#include "gdt.h"

#define USER_SELECTOR_RPL 0x3
#define USER_CODE_SELECTOR (GDT_USER_CODE | USER_SELECTOR_RPL)
#define USER_DATA_SELECTOR (GDT_USER_DATA | USER_SELECTOR_RPL)
#define USER_TEST_STACK_SIZE 4096u
#define PAGE_MASK (~(uintptr_t)(PMM_PAGE_SIZE - 1))

static uint8_t user_test_stack[USER_TEST_STACK_SIZE]
    __attribute__((aligned(PMM_PAGE_SIZE)));

static void map_user_test_page(uintptr_t vaddr, uint32_t flags)
{
    uintptr_t page = vaddr & PAGE_MASK;
    uintptr_t paddr;

    if (vmm_get_physaddr(page, &paddr) != 0)
        return;

    vmm_map_page(page, paddr & PAGE_MASK, flags);
}

void syscall_handler(uint32_t number)
{
    printk("syscall: number=%x\n", number);
}

static void user_test_entry(void)
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

    __asm__ volatile ("int $0x80" : : "a" (1) : "memory");

    volatile uint32_t *bad = (volatile uint32_t *)VMM_BOOTSTRAP_LIMIT;
    volatile uint32_t value = *bad;
    (void)value;

    for (;;)
        ;
}

void user_enter_syscall_test(void)
{
    uint32_t user_stack = (uint32_t)(user_test_stack + USER_TEST_STACK_SIZE);
    uint32_t eflags;

    map_user_test_page((uintptr_t)user_test_entry,
        VMM_PAGE_PRESENT | VMM_PAGE_USER);
    map_user_test_page((uintptr_t)user_test_stack,
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
          [user_eip] "r" ((uint32_t)user_test_entry)
        : "memory"
    );

    __builtin_unreachable();
}
