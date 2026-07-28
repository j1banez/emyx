#include <string.h>

#include <kernel/paging.h>
#include <kernel/panic.h>
#include <kernel/pmm.h>
#include <kernel/vmm.h>

#define PAGE_TABLE_SPAN (1024u * PMM_PAGE_SIZE)
#define VMM_PDE_INDEX(vaddr) (((vaddr) >> 22) & 0x3ff)
#define VMM_PTE_INDEX(vaddr) (((vaddr) >> 12) & 0x3ff)
// The upper 10 virtual-address bits select PDE 768 at 0xC0000000.
#define KERNEL_PDE_INDEX VMM_PDE_INDEX(VMM_KERNEL_VIRTUAL_BASE)

static uintptr_t page_directory;

static uint32_t *vmm_get_page_table_in(uintptr_t address_space,
    uintptr_t vaddr);

void *vmm_phys_to_virt(uintptr_t paddr)
{
    if (paddr >= VMM_DIRECT_MAP_LIMIT)
        return NULL;

    return (void *)(VMM_KERNEL_VIRTUAL_BASE + paddr);
}

uintptr_t vmm_virt_to_phys(const void *vaddr)
{
    uintptr_t addr;

    addr = (uintptr_t)vaddr;
    if (addr < VMM_KERNEL_VIRTUAL_BASE ||
            addr >= VMM_KERNEL_VIRTUAL_BASE + VMM_DIRECT_MAP_LIMIT)
        return 0;

    return addr - VMM_KERNEL_VIRTUAL_BASE;
}

void vmm_init(size_t limit)
{
    if (limit < 1)
        panic("vmm: limit must be more than 0");

    if (limit % PAGE_TABLE_SPAN != 0)
        panic("vmm: limit must be a multiple of page table span");

    page_directory = pmm_alloc_page();

    if (page_directory == 0)
        panic("vmm: page directory allocation failed");

    uint32_t *pd_ptr = vmm_phys_to_virt(page_directory);

    if (pd_ptr == NULL)
        panic("vmm: page directory is outside direct map");

    memset(pd_ptr, 0, PMM_PAGE_SIZE);

    for (size_t i = 0; i < limit / PAGE_TABLE_SPAN; i++) {
        uintptr_t page_table = pmm_alloc_page();
        uintptr_t kernel_page_table = pmm_alloc_page();

        if (page_table == 0 || kernel_page_table == 0)
            panic("vmm: page table allocation failed");

        uint32_t *pt_ptr = vmm_phys_to_virt(page_table);
        uint32_t *kernel_pt_ptr = vmm_phys_to_virt(kernel_page_table);

        if (pt_ptr == NULL || kernel_pt_ptr == NULL)
            panic("vmm: page table is outside direct map");

        memset(pt_ptr, 0, PMM_PAGE_SIZE);
        memset(kernel_pt_ptr, 0, PMM_PAGE_SIZE);

        for (size_t j = 0; j < 1024; j += 1) {
            uintptr_t frame = i * PAGE_TABLE_SPAN + j * PMM_PAGE_SIZE;
            pt_ptr[j] = frame | 0x3;
            kernel_pt_ptr[j] = frame | 0x3;
        }

        pd_ptr[i] = page_table | 0x3;
        pd_ptr[KERNEL_PDE_INDEX + i] = kernel_page_table | 0x3;
    }

    paging_load_directory(page_directory);
    paging_enable();
}

uintptr_t vmm_create_address_space(void)
{
    uintptr_t address_space;
    uint32_t *current_pd;
    uint32_t *new_pd;
    uint32_t i;

    address_space = pmm_alloc_page();
    if (address_space == 0)
        return 0;

    new_pd = vmm_phys_to_virt(address_space);
    if (new_pd == NULL) {
        pmm_free_page(address_space);
        return 0;
    }

    memset(new_pd, 0, PMM_PAGE_SIZE);

    current_pd = vmm_phys_to_virt(page_directory);
    for (i = KERNEL_PDE_INDEX; i < 1024; i++)
        new_pd[i] = current_pd[i];

    return address_space;
}

void vmm_destroy_address_space(uintptr_t address_space)
{
    uint32_t *pd_ptr;
    uint32_t i;

    if (address_space == 0 || address_space == page_directory)
        return;

    pd_ptr = vmm_phys_to_virt(address_space);
    if (pd_ptr == NULL)
        return;

    for (i = 0; i < KERNEL_PDE_INDEX; i++) {
        if ((pd_ptr[i] & VMM_PAGE_PRESENT) != 0)
            pmm_free_page(pd_ptr[i] & 0xfffff000);
    }

    pmm_free_page(address_space);
}

/*
 * Given a 32-bit virtual address:
 * [ pd_idx (10 bits) | pt_idx (10 bits) | offset (12 bits) ]
 * - pd_idx indexes page directory entries in the page directory.
 * - pt_idx indexes page table entries in one page table.
 * - offset is byte offset inside the final 4 KiB physical page frame.
 */
static uint32_t *vmm_get_page_table_in(uintptr_t address_space,
    uintptr_t vaddr)
{
    uint32_t *pd_ptr;

    if (address_space == 0)
        return NULL;

    pd_ptr = vmm_phys_to_virt(address_space);
    if (pd_ptr == NULL)
        return NULL;

    uint32_t pde = pd_ptr[VMM_PDE_INDEX(vaddr)];

    if ((pde & 0x1) == 0)
        return NULL;

    return vmm_phys_to_virt(pde & 0xfffff000);
}

int vmm_map_page(uintptr_t vaddr, uintptr_t paddr, uint32_t flags)
{
    uint32_t *pt_ptr;

    if ((vaddr & (PMM_PAGE_SIZE - 1)) != 0 ||
            (paddr & (PMM_PAGE_SIZE - 1)) != 0)
        return -1;

    pt_ptr = vmm_get_page_table_in(page_directory, vaddr);
    if (pt_ptr != NULL)
        pt_ptr[VMM_PTE_INDEX(vaddr)] = 0;

    return vmm_map_page_in(page_directory, vaddr, paddr, flags);
}

int vmm_map_page_in(uintptr_t address_space, uintptr_t vaddr,
    uintptr_t paddr, uint32_t flags)
{
    uint32_t *pd_ptr;
    uint32_t pd_idx;
    uint32_t pt_idx;
    uint32_t *pt_ptr;
    uintptr_t page_table;

    if (address_space == 0 || (vaddr & (PMM_PAGE_SIZE - 1)) != 0)
        return -1;
    if ((paddr & (PMM_PAGE_SIZE - 1)) != 0)
        return -1;

    pd_idx = VMM_PDE_INDEX(vaddr);
    if (address_space != page_directory && pd_idx >= KERNEL_PDE_INDEX)
        return -1;

    pd_ptr = vmm_phys_to_virt(address_space);
    if (pd_ptr == NULL)
        return -1;

    if ((pd_ptr[pd_idx] & VMM_PAGE_PRESENT) != 0) {
        pt_ptr = vmm_phys_to_virt(pd_ptr[pd_idx] & 0xfffff000);
        if (pt_ptr == NULL)
            return -1;
    } else {
        page_table = pmm_alloc_page();
        if (page_table == 0)
            return -1;

        pt_ptr = vmm_phys_to_virt(page_table);
        if (pt_ptr == NULL) {
            pmm_free_page(page_table);
            return -1;
        }

        memset(pt_ptr, 0, PMM_PAGE_SIZE);
        pd_ptr[pd_idx] = page_table | VMM_PAGE_PRESENT;
    }

    pt_idx = VMM_PTE_INDEX(vaddr);
    if ((pt_ptr[pt_idx] & VMM_PAGE_PRESENT) != 0)
        return -1;

    pd_ptr[pd_idx] |= flags & (VMM_PAGE_WRITABLE | VMM_PAGE_USER);
    pt_ptr[pt_idx] = paddr | (flags & 0xfff);
    paging_tlb_invalidate(vaddr);
    return 0;
}

int vmm_unmap_page(uintptr_t vaddr)
{
    return vmm_unmap_page_in(page_directory, vaddr);
}

int vmm_unmap_page_in(uintptr_t address_space, uintptr_t vaddr)
{
    uint32_t *pt_ptr;
    uint32_t pt_idx;

    if (address_space == 0 || (vaddr & (PMM_PAGE_SIZE - 1)) != 0)
        return -1;
    if (address_space != page_directory &&
            VMM_PDE_INDEX(vaddr) >= KERNEL_PDE_INDEX)
        return -1;

    pt_ptr = vmm_get_page_table_in(address_space, vaddr);

    if (pt_ptr == NULL)
        return -1;

    pt_idx = VMM_PTE_INDEX(vaddr);

    if ((pt_ptr[pt_idx] & VMM_PAGE_PRESENT) == 0)
        return -1;

    pt_ptr[pt_idx] = 0;
    paging_tlb_invalidate(vaddr);
    return 0;
}

int vmm_get_paddr(uintptr_t vaddr, uintptr_t *paddr)
{
    return vmm_get_paddr_in(page_directory, vaddr, paddr);
}

int vmm_get_paddr_in(uintptr_t address_space, uintptr_t vaddr,
    uintptr_t *paddr)
{
    uint32_t *pt_ptr = vmm_get_page_table_in(address_space, vaddr);

    if (paddr == NULL || pt_ptr == NULL)
        return -1;

    uint32_t pte = pt_ptr[VMM_PTE_INDEX(vaddr)];

    if ((pte & VMM_PAGE_PRESENT) == 0)
        return -1;

    *paddr = (pte & 0xfffff000) | (vaddr & 0xfff);
    return 0;
}
