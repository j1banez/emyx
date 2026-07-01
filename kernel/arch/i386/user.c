#include <stdint.h>

#include <kernel/syscall.h>
#include <kernel/user.h>

#include "gdt.h"

#define USER_SELECTOR_RPL 0x3
#define USER_CODE_SELECTOR (GDT_USER_CODE | USER_SELECTOR_RPL)
#define USER_DATA_SELECTOR (GDT_USER_DATA | USER_SELECTOR_RPL)
#define INIT_CODE_SIZE 49u
#define U32LE(value) \
    (uint8_t)((value) & 0xff), \
    (uint8_t)(((value) >> 8) & 0xff), \
    (uint8_t)(((value) >> 16) & 0xff), \
    (uint8_t)(((value) >> 24) & 0xff)

static const uint8_t init_image[] = {
    EMXF_MAGIC0, EMXF_MAGIC1, EMXF_MAGIC2, EMXF_MAGIC3,
    U32LE(INIT_CODE_SIZE),                       /* code size */
    U32LE(0u),                                   /* entry offset */
    0x66, 0xb8, USER_DATA_SELECTOR, 0x00,       /* mov ax, user data */
    0x8e, 0xd8,                                 /* mov ds, ax */
    0x8e, 0xc0,                                 /* mov es, ax */
    0x8e, 0xe0,                                 /* mov fs, ax */
    0x8e, 0xe8,                                 /* mov gs, ax */
    0xb8, U32LE(SYS_WRITE),                     /* mov eax, SYS_WRITE */
    0xbb, U32LE(SYS_FD_STDOUT),                 /* mov ebx, stdout */
    0xb9, U32LE(USER_INIT_DATA_ADDR),           /* mov ecx, message */
    0xba, U32LE(USER_INIT_MESSAGE_LEN),         /* mov edx, length */
    0xcd, 0x80,                                 /* int 0x80 */
    0xb8, U32LE(SYS_EXIT),                      /* mov eax, SYS_EXIT */
    0x31, 0xdb,                                 /* xor ebx, ebx */
    0x31, 0xc9,                                 /* xor ecx, ecx */
    0x31, 0xd2,                                 /* xor edx, edx */
    0xcd, 0x80,                                 /* int 0x80 */
    0xeb, 0xfe,                                 /* jmp . */
};

const void *user_init_image(void)
{
    return init_image;
}

size_t user_init_image_size(void)
{
    return sizeof(init_image);
}

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
