#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/pmm.h>
#include <kernel/user.h>
#include <kernel/vmm.h>

static const char init_message[] = "hello from user init\n";

static user_process init_process;
static user_process *current_process;
static uint32_t next_process_id;

static int map_copied_user_page(uintptr_t vaddr, const void *src, size_t size,
    uint32_t flags)
{
    uintptr_t paddr;

    if (size > PMM_PAGE_SIZE)
        return -1;

    paddr = pmm_alloc_page();
    if (paddr == 0)
        return -1;
    if (paddr >= VMM_BOOTSTRAP_LIMIT) {
        pmm_free_page(paddr);
        return -1;
    }

    memset((void *)paddr, 0, PMM_PAGE_SIZE);
    if (src != NULL)
        memcpy((void *)paddr, src, size);

    if (vmm_map_page(vaddr, paddr,
            VMM_PAGE_PRESENT | VMM_PAGE_USER | flags) != 0) {
        pmm_free_page(paddr);
        return -1;
    }

    return 0;
}

void user_run_init(void)
{
    user_process *process;

    process = user_process_create();
    if (process == NULL)
        return;

    if (user_prepare_init(process) != 0)
        return;

    user_enter(process);
}

user_process *user_process_create(void)
{
    init_process.id = next_process_id++;
    init_process.entry = 0;
    init_process.stack_top = 0;
    init_process.exit_status = 0;
    init_process.exited = 0;

    current_process = &init_process;
    return current_process;
}

user_process *user_current_process(void)
{
    return current_process;
}

int user_prepare_init(user_process *process)
{
    if (process == NULL)
        return -1;

    if (map_copied_user_page(USER_INIT_CODE_ADDR, user_init_code(),
            user_init_code_size(), 0) != 0)
        return -1;
    if (map_copied_user_page(USER_INIT_DATA_ADDR, init_message,
            USER_INIT_MESSAGE_LEN + 1, 0) != 0)
        return -1;
    if (map_copied_user_page(USER_INIT_STACK_TOP - PMM_PAGE_SIZE, NULL, 0,
            VMM_PAGE_WRITABLE) != 0)
        return -1;

    process->entry = USER_INIT_CODE_ADDR;
    process->stack_top = USER_INIT_STACK_TOP;

    return 0;
}

void user_exit_current(uint32_t status)
{
    if (current_process == NULL)
        return;

    current_process->exit_status = status;
    current_process->exited = 1;
}
