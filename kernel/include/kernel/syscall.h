#ifndef _KERNEL_SYSCALL_H
#define _KERNEL_SYSCALL_H

#include <stdint.h>

#define SYS_WRITE 1u
#define SYS_EXIT 2u
#define SYS_YIELD 3u
#define SYS_GETC 4u
#define SYS_SPAWN 5u

#define SYS_FD_STDOUT 1u
#define SYS_ERR ((uint32_t)-1)

struct syscall_frame {
    uint32_t edi;
    uint32_t esi;
    uint32_t ebp;
    uint32_t esp;
    uint32_t ebx;
    uint32_t edx;
    uint32_t ecx;
    uint32_t eax;
};

uint32_t syscall_dispatch(struct syscall_frame *frame);

#endif
