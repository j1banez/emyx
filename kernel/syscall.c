#include <stddef.h>
#include <stdint.h>

#include <kernel/printk.h>
#include <kernel/syscall.h>
#include <kernel/user.h>

static uint32_t sys_write(uint32_t fd, const char *buf, uint32_t len);
static void sys_exit(uint32_t status);

/*
 * int $0x80 syscall ABI:
 *   eax = syscall number on entry, return value on exit
 *   ebx = arg0, ecx = arg1, edx = arg2
 *   esi/edi are reserved for future arguments.
 */
uint32_t syscall_dispatch(struct syscall_frame *frame)
{
    switch (frame->eax) {
    case SYS_WRITE:
        return sys_write(frame->ebx, (const char *)frame->ecx,
            frame->edx);
    case SYS_EXIT:
        sys_exit(frame->ebx);
        return SYS_ERR;
    default:
        return SYS_ERR;
    }
}

static uint32_t sys_write(uint32_t fd, const char *buf, uint32_t len)
{
    uint32_t i;

    if (fd != SYS_FD_STDOUT || buf == NULL)
        return SYS_ERR;

    for (i = 0; i < len; i++)
        printk("%c", buf[i]);

    return len;
}

static void sys_exit(uint32_t status)
{
    user_exit_current(status);
    printk("user: exited status=%x\n", status);

    for (;;)
        ;
}
