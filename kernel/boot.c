#include <stddef.h>
#include <stdint.h>

#include <kernel/boot.h>
#include <kernel/printk.h>
#include <kernel/vmm.h>

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

    mods = vmm_phys_to_virt(mbi->mods_addr);
    if (mods == NULL) {
        printk("boot: module list is outside direct map\n");
        return;
    }

    module_start = vmm_phys_to_virt(mods[0].mod_start);
    if (module_start == NULL) {
        printk("boot: module is outside direct map\n");
        return;
    }
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
