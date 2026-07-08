#ifndef USER_USER_H
#define USER_USER_H

#include <stdint.h>

#define SYS_WRITE 1u
#define SYS_GETC 4u
#define SYS_SPAWN 5u

#define SYS_FD_STDOUT 1u

static inline int write(int fd, const void *buf, uint32_t len)
{
    uint32_t ret;

    __asm__ volatile (
        "int $0x80"
        : "=a" (ret)
        : "a" (SYS_WRITE), "b" ((uint32_t)fd), "c" ((uint32_t)buf),
          "d" (len)
        : "memory");

    return (int)ret;
}

static inline int getchar(void)
{
    uint32_t ret;

    __asm__ volatile (
        "int $0x80"
        : "=a" (ret)
        : "a" (SYS_GETC)
        : "memory");

    return (int)ret;
}

static inline int spawn(const char *path)
{
    uint32_t ret;

    __asm__ volatile (
        "int $0x80"
        : "=a" (ret)
        : "a" (SYS_SPAWN), "b" ((uint32_t)path)
        : "memory");

    return (int)ret;
}

#endif
