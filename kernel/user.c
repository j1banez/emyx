#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/endian.h>
#include <kernel/initramfs.h>
#include <kernel/keyboard.h>
#include <kernel/pmm.h>
#include <kernel/sched.h>
#include <kernel/user.h>
#include <kernel/vmm.h>

static const char init_message[] = "hello from user init\n";

static user_process exec_process;
static uint32_t next_process_id;
static uint8_t input_focus;
static char exec_path[EMXA_PATH_SIZE];

static int user_process_init(user_process *process);
static int user_prepare_exec(user_process *process, const char *path);
static void user_exec_task(void);

static void user_process_free_pages(user_process *process)
{
    uint32_t i;

    if (process == NULL)
        return;

    for (i = 0; i < process->page_count; i++) {
        vmm_unmap_page(process->pages[i].vaddr);
        pmm_free_page(process->pages[i].paddr);
        process->pages[i].vaddr = 0;
        process->pages[i].paddr = 0;
    }

    process->page_count = 0;
}

static int map_copied_user_page(user_process *process, uintptr_t vaddr,
    const void *src, size_t size, uint32_t flags)
{
    uintptr_t paddr;

    if (process == NULL || process->page_count >= USER_PROCESS_MAX_PAGES)
        return -1;
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

    process->pages[process->page_count].vaddr = vaddr;
    process->pages[process->page_count].paddr = paddr;
    process->page_count++;

    return 0;
}

static int load_emxf(user_process *process, const void *image, size_t size)
{
    const uint8_t *bytes;
    const uint8_t *code;
    uint32_t code_size;
    uint32_t entry_offset;

    if (process == NULL || image == NULL || size < EMXF_HEADER_SIZE)
        return -1;

    bytes = (const uint8_t *)image;
    if (bytes[0] != EMXF_MAGIC0 || bytes[1] != EMXF_MAGIC1 ||
            bytes[2] != EMXF_MAGIC2 || bytes[3] != EMXF_MAGIC3)
        return -1;
    code_size = read_le32(bytes + 4);
    entry_offset = read_le32(bytes + 8);

    if (code_size > PMM_PAGE_SIZE)
        return -1;
    if (entry_offset >= code_size)
        return -1;
    if (EMXF_HEADER_SIZE + code_size > size)
        return -1;

    code = bytes + EMXF_HEADER_SIZE;
    if (map_copied_user_page(process, USER_INIT_CODE_ADDR, code,
            code_size, 0) != 0)
        return -1;

    process->entry = USER_INIT_CODE_ADDR + entry_offset;

    return 0;
}

int user_spawn(const char *path)
{
    const void *emxf;
    uint32_t emxf_size;
    size_t path_len;
    int task;

    if (path == NULL)
        return -1;

    path_len = strlen(path);
    if (path_len == 0 || path_len >= EMXA_PATH_SIZE)
        return -1;
    if (initramfs_find(path, &emxf, &emxf_size) != 0)
        return -1;

    memcpy(exec_path, path, path_len + 1);
    keyboard_buffer_clear();

    task = kthread_create(user_exec_task);
    return task;
}

static void user_exec_task(void)
{
    if (user_process_init(&exec_process) != 0)
        return;

    if (user_prepare_exec(&exec_process, exec_path) != 0)
        return;

    input_focus = 1;
    user_enter(&exec_process);
}

static int user_process_init(user_process *process)
{
    if (process == NULL)
        return -1;

    process->id = next_process_id++;
    process->entry = 0;
    process->stack_top = 0;
    process->exit_status = 0;
    process->exited = 0;
    process->page_count = 0;

    return 0;
}

int user_prepare_exec(user_process *process, const char *path)
{
    const void *emxf;
    uint32_t emxf_size;

    if (process == NULL || path == NULL)
        return -1;

    if (initramfs_find(path, &emxf, &emxf_size) != 0)
        goto fail;
    if (load_emxf(process, emxf, emxf_size) != 0)
        goto fail;
    if (map_copied_user_page(process, USER_INIT_DATA_ADDR, init_message,
            USER_INIT_MESSAGE_LEN + 1, 0) != 0)
        goto fail;
    if (map_copied_user_page(process, USER_INIT_STACK_TOP - PMM_PAGE_SIZE,
            NULL, 0, VMM_PAGE_WRITABLE) != 0)
        goto fail;

    process->stack_top = USER_INIT_STACK_TOP;

    return 0;

fail:
    user_process_free_pages(process);
    return -1;
}

uint8_t user_has_input_focus(void)
{
    return input_focus;
}

void user_exit_current(uint32_t status)
{
    input_focus = 0;
    exec_process.exit_status = status;
    exec_process.exited = 1;
    user_process_free_pages(&exec_process);
}
