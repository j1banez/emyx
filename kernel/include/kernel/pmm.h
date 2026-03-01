#ifndef _PMM_H
#define _PMM_H

#include <stdint.h>

void pmm_init(void *mbi_ptr);
uint32_t pmm_get_total_pages(void);
uint32_t pmm_get_free_pages(void);
uintptr_t pmm_alloc_page(void);
void pmm_free_page(uintptr_t addr);

#endif
