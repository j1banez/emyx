#ifndef _PAGING_H
#define _PAGING_H

#include <stdint.h>

void paging_load_directory(uintptr_t pd_paddr);
void paging_enable(void);
uintptr_t paging_fault_addr(void);
void paging_tlb_invalidate(uintptr_t vaddr);

#endif
