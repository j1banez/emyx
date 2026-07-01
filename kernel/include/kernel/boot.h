#ifndef _KERNEL_BOOT_H
#define _KERNEL_BOOT_H

#include <stdint.h>

#define MULTIBOOT_MAGIC 0x2badb002
#define MULTIBOOT_FLAG_MMAP (1u << 6)
#define MULTIBOOT_FLAG_MODS (1u << 3)

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} multiboot_info;

typedef struct {
    uint32_t mod_start;
    uint32_t mod_end;
    uint32_t string;
    uint32_t reserved;
} multiboot_module;

typedef struct {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) multiboot_mmap_entry;

void boot_init(multiboot_info *mbi);
const void *boot_module_start(void);
uint32_t boot_module_size(void);

#endif
