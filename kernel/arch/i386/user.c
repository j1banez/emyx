#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/pmm.h>
#include <kernel/syscall.h>
#include <kernel/user.h>
#include <kernel/vmm.h>

#include "gdt.h"

#define USER_SELECTOR_RPL 0x3
#define USER_CODE_SELECTOR (GDT_USER_CODE | USER_SELECTOR_RPL)
#define USER_DATA_SELECTOR (GDT_USER_DATA | USER_SELECTOR_RPL)
#define USER_INIT_CODE_ADDR 0x00400000u
#define USER_INIT_DATA_ADDR 0x00401000u
#define USER_INIT_STACK_TOP 0x00800000u
#define U32LE(value) \
    (uint8_t)((value) & 0xff), \
    (uint8_t)(((value) >> 8) & 0xff), \
    (uint8_t)(((value) >> 16) & 0xff), \
    (uint8_t)(((value) >> 24) & 0xff)

static const char init_message[] = "hello from user init\n";

static const uint8_t user_init_code[] = {
    0x66, 0xb8, USER_DATA_SELECTOR, 0x00,       /* mov ax, user data */
    0x8e, 0xd8,                                 /* mov ds, ax */
    0x8e, 0xc0,                                 /* mov es, ax */
    0x8e, 0xe0,                                 /* mov fs, ax */
    0x8e, 0xe8,                                 /* mov gs, ax */
    0xb8, U32LE(SYS_WRITE),                     /* mov eax, SYS_WRITE */
    0xbb, U32LE(SYS_FD_STDOUT),                 /* mov ebx, stdout */
    0xb9, U32LE(USER_INIT_DATA_ADDR),           /* mov ecx, message */
    0xba, U32LE(sizeof(init_message) - 1),       /* mov edx, length */
    0xcd, 0x80,                                 /* int 0x80 */
    0xb8, U32LE(SYS_EXIT),                      /* mov eax, SYS_EXIT */
    0x31, 0xdb,                                 /* xor ebx, ebx */
    0x31, 0xc9,                                 /* xor ecx, ecx */
    0x31, 0xd2,                                 /* xor edx, edx */
    0xcd, 0x80,                                 /* int 0x80 */
    0xeb, 0xfe,                                 /* jmp . */
};

static int map_copied_user_page(uintptr_t vaddr, const void *src, size_t size,
    uint32_t flags)
{
    uintptr_t paddr;

    if (size > PMM_PAGE_SIZE)
        return -1;

    paddr = pmm_alloc_page();
    if (paddr == 0)
        return -1;
    if (paddr >= VMM_BOOTSTRAP_LIMIT) {
        pmm_free_page(paddr);
        return -1;
    }

    memset((void *)paddr, 0, PMM_PAGE_SIZE);
    if (src != NULL)
        memcpy((void *)paddr, src, size);

    if (vmm_map_page(vaddr, paddr,
            VMM_PAGE_PRESENT | VMM_PAGE_USER | flags) != 0) {
        pmm_free_page(paddr);
        return -1;
    }

    return 0;
}

void user_run_init(void)
{
    uint32_t user_stack = USER_INIT_STACK_TOP;
    uint32_t eflags;

    if (map_copied_user_page(USER_INIT_CODE_ADDR, user_init_code,
            sizeof(user_init_code), 0) != 0)
        return;
    if (map_copied_user_page(USER_INIT_DATA_ADDR, init_message,
            sizeof(init_message), 0) != 0)
        return;
    if (map_copied_user_page(USER_INIT_STACK_TOP - PMM_PAGE_SIZE, NULL, 0,
            VMM_PAGE_WRITABLE) != 0)
        return;

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
          [user_eip] "r" (USER_INIT_CODE_ADDR)
        : "memory"
    );

    __builtin_unreachable();
}
