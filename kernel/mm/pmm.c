#include <kernel/panic.h>
#include <kernel/pmm.h>
#include <kernel/printk.h>

#define MULTIBOOT_FLAG_MMAP (1u << 6)

extern uint8_t __kernel_start[];
extern uint8_t __kernel_end[];

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

static uint8_t *pmm_bitmap;
static uint32_t pmm_bitmap_bytes;
static uint32_t pmm_max_page;
static uint32_t pmm_total_pages;
static uint32_t pmm_free_pages;

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

static uintptr_t align_up(uintptr_t addr, uintptr_t size)
{
    return (addr + size - 1) & ~(size - 1);
}

static void reserve_page(uint32_t page)
{
    if (page >= pmm_max_page)
        return;
    if (bitmap_test(page) == 0) {
        bitmap_set(page);
        if (pmm_free_pages > 0)
            pmm_free_pages--;
    }
}

/*
 * Find pages that correspond to given addresses and reserve so they cannot
 * be allocated anymore.
 * [start_addr, end_addr)
 */
static void reserve_segment(uintptr_t start_addr, uintptr_t end_addr)
{
    uint32_t start_page = start_addr >> PMM_PAGE_SHIFT;
    uint32_t end_page = (end_addr + PMM_PAGE_SIZE - 1) >> PMM_PAGE_SHIFT;

    for (uint32_t page = start_page; page < end_page; page++) {
        reserve_page(page);
    }
}

/*
 * Checks mmap entries and panic if one is malformed.
 * Call this function before any logic that is using mmap.
 */
static void check_mmap(multiboot_info *mbi)
{
    if (!(mbi->flags & MULTIBOOT_FLAG_MMAP))
        panic("multiboot: mmap not present\n");

    uint8_t *ptr = (uint8_t *)(uintptr_t)mbi->mmap_addr;
    uint8_t *end = ptr + mbi->mmap_length;

    while (ptr < end) {
        multiboot_mmap_entry *entry = (multiboot_mmap_entry *)ptr;

        if (entry->size < 20)
            panic("multiboot: mmap entry size too small\n");

        printk("pmm: entry addr=%x%x len=%x%x type=%u\n",
            (uint32_t)(entry->addr >> 32), (uint32_t)entry->addr,
            (uint32_t)(entry->len >> 32), (uint32_t)entry->len,
            entry->type);

        ptr += entry->size + sizeof(entry->size);
    }
}

/*
 * Update bitmap so all pages inside type 1 regions can be allocated
 */
static uint32_t clear_usable_pages(multiboot_info *mbi)
{
    uint8_t *ptr = (uint8_t *)(uintptr_t)mbi->mmap_addr;
    uint8_t *end = ptr + mbi->mmap_length;
    uint32_t usable_pages = 0;

    while (ptr < end) {
        multiboot_mmap_entry *entry = (multiboot_mmap_entry *)ptr;

        if (entry->type == 1) {
            uint32_t start_page = (uint32_t)(entry->addr >> PMM_PAGE_SHIFT);
            uint32_t region_pages = (uint32_t)(entry->len >> PMM_PAGE_SHIFT);
            uint32_t end_page = start_page + region_pages;

            if (start_page >= pmm_max_page) {
                ptr += entry->size + sizeof(entry->size);
                continue;
            }

            if (end_page > pmm_max_page || end_page < start_page)
                end_page = pmm_max_page;

            for (uint32_t i = start_page; i < end_page; i++) {
                bitmap_clear(i);
            }

            usable_pages += end_page - start_page;
        }

        ptr += entry->size + sizeof(entry->size);
    }

    return usable_pages;
}

/*
 * Finds the last page index, the one at the end of the last usable region.
 */
static uint32_t find_max_page(multiboot_info *mbi)
{
    uint8_t *ptr = (uint8_t *)(uintptr_t)mbi->mmap_addr;
    uint8_t *end = ptr + mbi->mmap_length;
    uint64_t max_page = 0;

    while (ptr < end) {
        multiboot_mmap_entry *entry = (multiboot_mmap_entry *)ptr;

        if (entry->type == 1) {
            uint64_t entry_max_page = (entry->addr + entry->len) >> PMM_PAGE_SHIFT;

            if (entry_max_page > max_page)
                max_page = entry_max_page;
        }

        ptr += entry->size + sizeof(entry->size);
    }

    return max_page;
}

static uint64_t overlap_bytes(
    multiboot_mmap_entry *entry, uint64_t start, uint64_t end
)
{
    if (entry->type != 1)
        return 0;

    uint64_t estart = entry->addr;
    uint64_t eend = entry->addr + entry->len;

    uint64_t max_start = (estart > start) ? estart : start;
    uint64_t min_end = (eend < end) ? eend : end;

    if (max_start >= min_end)
        return 0;

    return min_end - max_start;
}

static int ranges_overlap(
    uintptr_t a_start, uintptr_t a_end,
    uintptr_t b_start, uintptr_t b_end)
{
    return a_start < b_end && b_start < a_end;
}

/*
 * Find a memory segment that can contain enough bytes in a type 1 region.
 * Guarantees segment is not overlaping kernel.
 * Guarantees segment is not overlaping multiboot data.
 */
static uintptr_t find_segment(multiboot_info *mbi, uint32_t bytes)
{
    uint8_t *ptr = (uint8_t *)(uintptr_t)mbi->mmap_addr;
    uint8_t *end = ptr + mbi->mmap_length;

    while (ptr < end) {
        multiboot_mmap_entry *entry = (multiboot_mmap_entry *)ptr;

        if (entry->type != 1) {
            ptr += entry->size + sizeof(entry->size);
            continue;
        }

        // Avoid null pointer
        uintptr_t estart = entry->addr == 0 ? 1 : (uintptr_t)entry->addr;
        uintptr_t eend = (uintptr_t)(entry->addr + entry->len);
        uintptr_t start = align_up(estart, PMM_PAGE_SIZE);

        while (start + bytes <= eend) {
            uintptr_t kstart = (uintptr_t)__kernel_start;
            uintptr_t kend = (uintptr_t)__kernel_end;

            // Current range is overlaping kernel
            if (ranges_overlap(start, start + bytes, kstart, kend)) {
                start = align_up(kend, PMM_PAGE_SIZE);
                continue;
            }

            uintptr_t mbi_start = (uintptr_t)mbi;
            uintptr_t mbi_end = mbi_start + sizeof(*mbi);

            // Current range is overlaping multiboot info
            if (ranges_overlap(start, start + bytes, mbi_start, mbi_end)) {
                start = align_up(mbi_end, PMM_PAGE_SIZE);
                continue;
            }

            uintptr_t mmap_start = (uintptr_t)mbi->mmap_addr;
            uintptr_t mmap_end = mmap_start + mbi->mmap_length;

            // Current range is overlaping mmap
            if (ranges_overlap(start, start + bytes, mmap_start, mmap_end)) {
                start = align_up(mmap_end, PMM_PAGE_SIZE);
                continue;
            }

            return start;
        }

        ptr += entry->size + sizeof(entry->size);
    }

    return 0;
}

/*
 * Checks that kernel is located in one or more type 1 regions
 */
static void check_kernel_location(multiboot_info *mbi)
{
    uint8_t *ptr = (uint8_t *)(uintptr_t)mbi->mmap_addr;
    uint8_t *end = ptr + mbi->mmap_length;
    uint64_t kstart = (uint64_t)(uintptr_t)__kernel_start;
    uint64_t kend = (uint64_t)(uintptr_t)__kernel_end;
    uint64_t kernel_overlap = 0;

    while (ptr < end) {
        multiboot_mmap_entry *entry = (multiboot_mmap_entry *)ptr;

        kernel_overlap += overlap_bytes(entry, kstart, kend);

        ptr += entry->size + sizeof(entry->size);
    }

    if (kernel_overlap < (kend - kstart))
        panic("pmm: kernel not fully located in usable RAM");
}

void pmm_init(void *mbi_ptr)
{
    multiboot_info *mbi = (multiboot_info *)mbi_ptr;

    check_mmap(mbi);
    pmm_max_page = find_max_page(mbi);
    pmm_bitmap_bytes = (pmm_max_page + 7) / 8;

    uintptr_t bitmap_base = find_segment(mbi, pmm_bitmap_bytes);

    if (bitmap_base == 0)
        panic("pmm: pages bitmap does not fit in RAM\n");

    check_kernel_location(mbi);

    pmm_bitmap = (uint8_t *)bitmap_base;

    for (uint32_t i = 0; i < pmm_bitmap_bytes; i++) {
        pmm_bitmap[i] = 0xFF;
    }

    uint32_t usable_pages = clear_usable_pages(mbi);

    pmm_total_pages = usable_pages;
    pmm_free_pages = usable_pages;

    reserve_segment(bitmap_base, bitmap_base + pmm_bitmap_bytes);
    reserve_segment((uintptr_t)__kernel_start, (uintptr_t)__kernel_end);

    // Reserve page 0 so memory address 0 (null pointer) is not allocatable.
    reserve_page(0);

    printk("pmm: pages total=%u free=%u\n",
        pmm_get_total_pages(), pmm_get_free_pages());
}

uintptr_t pmm_alloc_page(void)
{
    for (uint32_t i = 1; i < pmm_max_page; i++) {
        if (bitmap_test(i) == 0) {
            bitmap_set(i);
            if (pmm_free_pages > 0) pmm_free_pages--;
            return i << PMM_PAGE_SHIFT;
        }
    }

    return 0;
}

void pmm_free_page(uintptr_t addr)
{
    uint32_t page = addr >> PMM_PAGE_SHIFT;

    if (page != 0 && page < pmm_max_page && bitmap_test(page)) {
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
