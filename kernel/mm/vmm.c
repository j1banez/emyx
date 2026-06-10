#include <string.h>

#include <kernel/paging.h>
#include <kernel/panic.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>

#define PAGE_TABLE_SPAN (1024u * PMM_PAGE_SIZE)
#define VMM_PDE_INDEX(vaddr) (((vaddr) >> 22) & 0x3ff)
#define VMM_PTE_INDEX(vaddr) (((vaddr) >> 12) & 0x3ff)

static uintptr_t page_directory;

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
static uint32_t *vmm_get_page_table(uintptr_t vaddr)
{
    uint32_t *pd_ptr = (uint32_t *)page_directory;
    uint32_t pde = pd_ptr[VMM_PDE_INDEX(vaddr)];

    if ((pde & 0x1) == 0)
        return 0;

    return (uint32_t *)(uintptr_t)(pde & 0xfffff000);
}

int vmm_map_page(uintptr_t vaddr, uintptr_t paddr, uint32_t flags)
{
    uint32_t *pd_ptr;
    uint32_t pd_idx;

    // Check that addresses are page aligned
    if ((vaddr & (PMM_PAGE_SIZE - 1)) != 0)
        return -1;
    if ((paddr & (PMM_PAGE_SIZE - 1)) != 0)
        return -1;

    uint32_t *pt_ptr = vmm_get_page_table(vaddr);

    if (pt_ptr == 0)
        return -1;

    pd_ptr = (uint32_t *)page_directory;
    pd_idx = VMM_PDE_INDEX(vaddr);
    pd_ptr[pd_idx] |= flags & 0xfff;

    pt_ptr[VMM_PTE_INDEX(vaddr)] = paddr | (flags & 0xfff);
    paging_tlb_invalidate(vaddr);
    return 0;
}

int vmm_unmap_page(uintptr_t vaddr)
{
    if ((vaddr & (PMM_PAGE_SIZE - 1)) != 0)
        return -1;

    uint32_t *pt_ptr = vmm_get_page_table(vaddr);

    if (pt_ptr == 0)
        return -1;

    uint32_t pt_idx = VMM_PTE_INDEX(vaddr);

    if ((pt_ptr[pt_idx] & VMM_PAGE_PRESENT) == 0)
        return -1;

    pt_ptr[pt_idx] = 0;
    paging_tlb_invalidate(vaddr);
    return 0;
}

int vmm_get_physaddr(uintptr_t vaddr, uintptr_t *paddr)
{
    uint32_t *pt_ptr = vmm_get_page_table(vaddr);

    if (paddr == 0 || pt_ptr == 0)
        return -1;

    uint32_t pte = pt_ptr[VMM_PTE_INDEX(vaddr)];

    if ((pte & VMM_PAGE_PRESENT) == 0)
        return -1;

    *paddr = (pte & 0xfffff000) | (vaddr & 0xfff);
    return 0;
}
