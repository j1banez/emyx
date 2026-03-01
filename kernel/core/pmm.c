#include <kernel/panic.h>
#include <kernel/pmm.h>
#include <kernel/printk.h>

#define MULTIBOOT_FLAG_MMAP (1u << 6)
#define PAGE_SHIFT 12 // 4 KiB (4096) pages
#define PMM_BITMAP_BYTES 131072

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
} multiboot_info;

typedef struct {
    uint32_t size;
    uint64_t addr;
    uint64_t len;
    uint32_t type;
} __attribute__((packed)) multiboot_mmap_entry;

static uint32_t pmm_total_pages;
static uint32_t pmm_free_pages;

/* TODO: Replace fixed-size static bitmap with a placed bitmap.
 * During pmm_init(), compute required bitmap bytes from detected page count,
 * place bitmap inside a usable RAM region, and reserve those bitmap pages.
 * This removes hardcoded RAM limits and avoids wasting .bss on small systems.
 */
static uint8_t pmm_bitmap[PMM_BITMAP_BYTES];

static void bitmap_set(uint32_t page)
{
    pmm_bitmap[page / 8] |= 1u << (page % 8);
}

static void bitmap_clear(uint32_t page)
{
    pmm_bitmap[page / 8] &= ~(1u << (page % 8));
}

static uint8_t bitmap_test(uint32_t page)
{
    return pmm_bitmap[page / 8] & (1u << (page % 8));
}

static uint32_t handle_entry(multiboot_mmap_entry *entry, uint32_t max_pages)
{
    if (entry->size < 20) {
        panic("multiboot: mmap entry size too small\n");
    }

    printk("pmm: entry addr=%x%x len=%x%x type=%u\n",
        (uint32_t)(entry->addr >> 32), (uint32_t)entry->addr,
        (uint32_t)(entry->len >> 32), (uint32_t)entry->len,
        entry->type);

    if (entry->type == 1) {
        uint32_t start_page = (uint32_t)(entry->addr >> PAGE_SHIFT);
        uint32_t region_pages = (uint32_t)(entry->len >> PAGE_SHIFT);
        uint32_t end_page = start_page + region_pages;

        if (start_page >= max_pages) {
            return 0;
        }

        if (end_page > max_pages || end_page < start_page) {
            end_page = max_pages;
        }

        for (uint32_t i = start_page; i < end_page; i++) {
            bitmap_clear(i);
        }

        return end_page - start_page;
    }

    return 0;
}

void pmm_init(void *mbi_ptr)
{
    multiboot_info *mbi = (multiboot_info *)mbi_ptr;

    if (!(mbi->flags & MULTIBOOT_FLAG_MMAP)) {
        panic("multiboot: mmap not present\n");
    }

    for (int i = 0; i < PMM_BITMAP_BYTES; i++) {
        pmm_bitmap[i] = 0xFF;
    }

    uint8_t *ptr = (uint8_t *)(uintptr_t)mbi->mmap_addr;
    uint8_t *end = ptr + mbi->mmap_length;
    uint32_t total_regions = 0;
    uint32_t usable_pages = 0;
    uint32_t max_pages = PMM_BITMAP_BYTES * 8;

    while (ptr < end) {
        total_regions++;
        multiboot_mmap_entry *entry = (multiboot_mmap_entry *)ptr;

        usable_pages += handle_entry(entry, max_pages);

        ptr += entry->size + sizeof(entry->size);
    }

    bitmap_set(0);

    pmm_total_pages = usable_pages;
    pmm_free_pages = (usable_pages > 0) ? (usable_pages - 1) : 0;

    printk("pmm: regions=%u usable_pages=%u\n", total_regions, usable_pages);
    printk("pmm: total=%u free=%u\n",
        pmm_get_total_pages(), pmm_get_free_pages());
}

uintptr_t pmm_alloc_page(void)
{
    for (uint32_t i = 1; i < PMM_BITMAP_BYTES * 8; i++) {
        if (bitmap_test(i) == 0) {
            bitmap_set(i);
            if (pmm_free_pages > 0) pmm_free_pages--;
            return i << PAGE_SHIFT;
        }
    }

    return 0;
}

void pmm_free_page(uintptr_t addr)
{
    uint32_t page = addr >> PAGE_SHIFT;

    if (page != 0 && page < PMM_BITMAP_BYTES * 8 && bitmap_test(page)) {
        bitmap_clear(page);
        pmm_free_pages++;
    }
}

uint32_t pmm_get_total_pages(void)
{
    return pmm_total_pages;
}

uint32_t pmm_get_free_pages(void)
{
    return pmm_free_pages;
}
