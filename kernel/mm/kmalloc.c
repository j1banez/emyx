#include <kernel/kmalloc.h>
#include <kernel/pmm.h>

#include <stdint.h>

#define KMALLOC_ALIGN 8u

typedef struct heap_block {
    size_t size;
    uint8_t free;
    struct heap_block *next;
} heap_block;

static heap_block *list = NULL;

static int align_size(size_t size, size_t *aligned)
{
    if (size == 0)
        return -1;

    if (size > (size_t)-1 - (KMALLOC_ALIGN - 1))
        return -1;

    *aligned = (size + KMALLOC_ALIGN - 1) & ~(KMALLOC_ALIGN - 1);
    return 0;
}

static heap_block *new_block(void)
{
    heap_block *block = (heap_block *)pmm_alloc_page();

    if (!block)
        return NULL;

    block->size = PMM_PAGE_SIZE - sizeof(heap_block);
    block->free = 1;
    block->next = NULL;

    return block;
}

static void *alloc_from_block(heap_block *block, size_t size)
{
    size_t remaining = block->size - size;

    if (remaining >= sizeof(heap_block) + KMALLOC_ALIGN) {
        heap_block *next = (heap_block *)((uint8_t *)(block + 1) + size);

        next->free = 1;
        next->size = remaining - sizeof(heap_block);
        next->next = block->next;
        block->next = next;
    }

    block->free = 0;
    block->size = size;

    return block + 1;
}

static void coalesce_next(heap_block *block)
{
    while (block->next != NULL && block->next->free) {
        block->size += sizeof(heap_block) + block->next->size;
        block->next = block->next->next;
    }
}

void *kmalloc(size_t size)
{
    size_t asize;
    heap_block *current;
    heap_block *last;

    if (align_size(size, &asize) != 0)
        return NULL;

    // We do not handle sizes bigger than page size for now.
    // Will require pmm contiguous pages or vmm contiguous mapping.
    if (asize > PMM_PAGE_SIZE - sizeof(heap_block))
        return NULL;

    if (list == NULL) {
        list = new_block();
        if (list == NULL)
            return NULL;
    }

    current = list;
    last = NULL;

    while (current != NULL) {
        if (current->free && current->size >= asize)
            return alloc_from_block(current, asize);

        last = current;
        current = current->next;
    }

    current = new_block();

    if (current == NULL)
        return NULL;

    last->next = current;

    return alloc_from_block(current, asize);
}

void kfree(void *ptr)
{
    if (ptr == NULL)
        return;

    heap_block *block = ((heap_block *)ptr) - 1;
    block->free = 1;
    coalesce_next(block);
}
