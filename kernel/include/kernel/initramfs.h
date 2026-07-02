#ifndef _KERNEL_INITRAMFS_H
#define _KERNEL_INITRAMFS_H

#include <stdint.h>

#define EMXA_HEADER_SIZE 8u
#define EMXA_ENTRY_SIZE 40u
#define EMXA_PATH_SIZE 32u
#define EMXA_MAGIC0 'E'
#define EMXA_MAGIC1 'M'
#define EMXA_MAGIC2 'X'
#define EMXA_MAGIC3 'A'

int initramfs_find(const char *path, const void **data, uint32_t *size);
void initramfs_list(void);

#endif
