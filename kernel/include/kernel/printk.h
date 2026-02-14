#ifndef _KERNEL_PRINTK_H
#define _KERNEL_PRINTK_H

#include <stdint.h>

int printk(const char* format, ...);
void u32_to_hex(char out[11], uint32_t value);

#endif
