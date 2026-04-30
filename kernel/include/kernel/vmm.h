#ifndef _VMM_H
#define _VMM_H

#include <stddef.h>
#include <stdint.h>

#define VMM_BOOTSTRAP_LIMIT (16u * 1024u * 1024u)
#define VMM_PAGE_PRESENT 0x1u
#define VMM_PAGE_WRITABLE 0x2u
#define VMM_PAGE_USER 0x4u

void vmm_init(size_t limit);
int vmm_map_page(uintptr_t vaddr, uintptr_t paddr, uint32_t flags);
int vmm_unmap_page(uintptr_t vaddr);
int vmm_get_physaddr(uintptr_t vaddr, uintptr_t *paddr);

#endif
