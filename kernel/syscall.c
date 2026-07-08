#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/initramfs.h>
#include <kernel/keyboard.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/syscall.h>
#include <kernel/user.h>

static uint32_t sys_write(uint32_t fd, const char *buf, uint32_t len);
static void sys_exit(uint32_t status);
static uint32_t sys_yield(void);
static uint32_t sys_spawn(const char *path);
static int copy_string_from_user(char *dst, const char *src, uint32_t max_len);

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
    case SYS_YIELD:
        return sys_yield();
    case SYS_GETC:
        return keyboard_buffer_pop();
    case SYS_SPAWN:
        return sys_spawn((const char *)frame->ebx);
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

    kthread_exit();
}

static uint32_t sys_yield(void)
{
    sched_yield();
    return 0;
}

static uint32_t sys_spawn(const char *path)
{
    char kernel_path[EMXA_PATH_SIZE];
    int task;

    if (copy_string_from_user(kernel_path, path, EMXA_PATH_SIZE) != 0)
        return SYS_ERR;

    task = user_spawn(kernel_path);
    if (task < 0)
        return SYS_ERR;

    return (uint32_t)task;
}

static int copy_string_from_user(char *dst, const char *src, uint32_t max_len)
{
    uint32_t i;

    if (dst == NULL || src == NULL || max_len == 0)
        return -1;

    for (i = 0; i < max_len; i++) {
        dst[i] = src[i];
        if (dst[i] == '\0') {
            if (i == 0)
                return -1;
            return 0;
        }
    }

    dst[max_len - 1] = '\0';
    return -1;
}
