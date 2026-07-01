#include <stddef.h>
#include <stdint.h>

#include <kernel/boot.h>
#include <kernel/printk.h>

static const void *module_start;
static uint32_t module_size;

void boot_init(multiboot_info *mbi)
{
    multiboot_module *mods;

    module_start = NULL;
    module_size = 0;

    if ((mbi->flags & MULTIBOOT_FLAG_MODS) == 0 || mbi->mods_count == 0) {
        printk("boot: no modules\n");
        return;
    }

    mods = (multiboot_module *)(uintptr_t)mbi->mods_addr;
    module_start = (const void *)(uintptr_t)mods[0].mod_start;
    module_size = mods[0].mod_end - mods[0].mod_start;

    printk("boot: module start=%x size=%x\n", module_start, module_size);
}

const void *boot_module_start(void)
{
    return module_start;
}

uint32_t boot_module_size(void)
{
    return module_size;
}
