#include <kernel/paging.h>

void paging_load_directory(uintptr_t pd_paddr)
{
    __asm__ volatile("mov %0, %%cr3" :: "r"(pd_paddr) : "memory");
}

void paging_enable(void)
{
    uintptr_t cr0;

    __asm__ volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000u; // CR0.PG
    __asm__ volatile("mov %0, %%cr0" :: "r"(cr0) : "memory");
}

uintptr_t paging_fault_addr(void)
{
    uintptr_t cr2;

    __asm__ volatile("mov %%cr2, %0" : "=r"(cr2));
    return cr2;
}

void paging_tlb_invalidate(uintptr_t vaddr)
{
    __asm__ volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
}
