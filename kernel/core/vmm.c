#include <string.h>

#include <kernel/paging.h>
#include <kernel/panic.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>

#define PAGE_TABLE_SPAN (1024u * PMM_PAGE_SIZE)

uintptr_t page_directory;

void vmm_init(size_t limit)
{
    if (limit < 1)
        panic("vmm: limit must be more than 0");

    if (limit % PAGE_TABLE_SPAN != 0)
        panic("vmm: limit must be a multiple of page table span");

    page_directory = pmm_alloc_page();

    if (page_directory == 0)
        panic("vmm: page directory allocation failed");

    memset((void *)page_directory, 0, PMM_PAGE_SIZE);

    uint32_t *pd_ptr = (uint32_t *)page_directory;

    for (size_t i = 0; i < limit / PAGE_TABLE_SPAN; i++) {
        uintptr_t page_table = pmm_alloc_page();

        if (page_table == 0)
            panic("vmm: page table allocation failed");

        memset((void *)page_table, 0, PMM_PAGE_SIZE);

        uint32_t *pt_ptr = (uint32_t *)page_table;

        for (size_t j = 0; j < 1024; j += 1) {
            uintptr_t frame = i * PAGE_TABLE_SPAN + j * PMM_PAGE_SIZE;
            pt_ptr[j] = frame | 0x3;
        }

        pd_ptr[i] = page_table | 0x3;
    }

    paging_load_directory(page_directory);
    paging_enable();
}

/*
 * Given a 32-bit virtual address:
 * [ pd_idx (10 bits) | pt_idx (10 bits) | offset (12 bits) ]
 * - pd_idx indexes page directory entries in the page directory.
 * - pt_idx indexes page table entries in one page table.
 * - offset is byte offset inside the final 4 KiB physical page frame.
 */
