#ifndef _VMM_H
#define _VMM_H

#include <stddef.h>
#include <stdint.h>

#define VMM_KERNEL_VIRTUAL_BASE 0xC0000000u
#define VMM_DIRECT_MAP_LIMIT (16u * 1024u * 1024u)
#define VMM_PAGE_PRESENT 0x1u
#define VMM_PAGE_WRITABLE 0x2u
#define VMM_PAGE_USER 0x4u

void vmm_init(size_t limit);
void *vmm_phys_to_virt(uintptr_t paddr);
int vmm_map_page(uintptr_t vaddr, uintptr_t paddr, uint32_t flags);
int vmm_unmap_page(uintptr_t vaddr);
int vmm_get_physaddr(uintptr_t vaddr, uintptr_t *paddr);

#endif
