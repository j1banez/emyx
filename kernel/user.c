#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <kernel/endian.h>
#include <kernel/initramfs.h>
#include <kernel/keyboard.h>
#include <kernel/pmm.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/user.h>
#include <kernel/vmm.h>

static user_process processes[USER_PROCESS_MAX];
static uint32_t next_pid = 1u;
static uint8_t userland_active;

static user_process *user_process_alloc(void);
static user_process *user_process_find_by_pid(uint32_t pid);
static user_process *user_process_find_by_task_id(uint32_t task_id);
static void user_process_release(user_process *process);
static void user_process_orphan_children(user_process *parent);
static int user_prepare_exec(user_process *process, const char *path);
static void user_exec_task(void);

static user_process *user_process_alloc(void)
{
    uint32_t i;
    user_process *process;

    for (i = 0; i < USER_PROCESS_MAX; i++) {
        process = &processes[i];
        if (process->state != USER_PROCESS_FREE)
            continue;

        memset(process, 0, sizeof(*process));
        process->state = USER_PROCESS_CREATED;
        process->parent_pid = USER_PROCESS_NO_PARENT;
        return process;
    }

    return NULL;
}

static user_process *user_process_find_by_pid(uint32_t pid)
{
    uint32_t i;

    for (i = 0; i < USER_PROCESS_MAX; i++) {
        if (processes[i].state != USER_PROCESS_FREE &&
                processes[i].pid == pid)
            return &processes[i];
    }

    return NULL;
}

static user_process *user_process_find_by_task_id(uint32_t task_id)
{
    uint32_t i;

    for (i = 0; i < USER_PROCESS_MAX; i++) {
        if (processes[i].state != USER_PROCESS_FREE &&
                processes[i].task_id == task_id)
            return &processes[i];
    }

    return NULL;
}

static void user_process_release(user_process *process)
{
    if (process == NULL)
        return;

    // Zeroing the record sets state to USER_PROCESS_FREE, whose value is zero.
    memset(process, 0, sizeof(*process));
}

static void user_process_orphan_children(user_process *parent)
{
    uint32_t i;
    user_process *child;

    if (parent == NULL)
        return;

    for (i = 0; i < USER_PROCESS_MAX; i++) {
        child = &processes[i];
        if (child->state == USER_PROCESS_FREE ||
                child->parent_pid != parent->pid)
            continue;

        child->parent_pid = USER_PROCESS_NO_PARENT;
        if (child->state == USER_PROCESS_EXITED)
            user_process_release(child);
    }
}

static void user_process_free_pages(user_process *process)
{
    uint32_t i;

    if (process == NULL)
        return;

    for (i = 0; i < process->page_count; i++) {
        vmm_unmap_page_in(process->address_space,
            process->pages[i].vaddr);
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
    void *page;

    if (process == NULL || process->page_count >= USER_PROCESS_MAX_PAGES)
        return -1;
    if (size > PMM_PAGE_SIZE)
        return -1;

    paddr = pmm_alloc_page();
    if (paddr == 0)
        return -1;

    page = vmm_phys_to_virt(paddr);
    if (page == NULL) {
        pmm_free_page(paddr);
        return -1;
    }

    memset(page, 0, PMM_PAGE_SIZE);
    if (src != NULL)
        memcpy(page, src, size);

    if (vmm_map_page_in(process->address_space, vaddr, paddr,
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

void user_init(void)
{
    int pid;

    pid = user_spawn("/bin/init");
    if (pid < 0) {
        printk("user: failed to start /bin/init\n");
        return;
    }

    userland_active = 1;
    printk("user: init pid=%x\n", (uint32_t)pid);
    while (user_process_find_by_pid((uint32_t)pid) != NULL)
        sched_yield();

    userland_active = 0;
    printk("user: exiting userland\n");
}

int user_spawn(const char *path)
{
    const void *emxf;
    uint32_t emxf_size;
    size_t path_len;
    user_process *parent;
    user_process *process;
    int task_id;

    if (path == NULL)
        return -1;

    path_len = strlen(path);
    if (path_len == 0 || path_len >= EMXA_PATH_SIZE)
        return -1;
    if (initramfs_find(path, &emxf, &emxf_size) != 0)
        return -1;

    parent = user_process_find_by_task_id(sched_current_task_id());

    process = user_process_alloc();
    if (process == NULL)
        return -1;

    process->pid = next_pid++;
    if (parent != NULL)
        process->parent_pid = parent->pid;
    memcpy(process->path, path, path_len + 1);

    process->address_space = vmm_create_address_space();
    if (process->address_space == 0)
        goto fail;

    task_id = kthread_create(user_exec_task);
    if (task_id < 0)
        goto fail;

    process->task_id = (uint32_t)task_id;
    keyboard_buffer_clear();
    return (int)process->pid;

fail:
    if (process->address_space != 0)
        vmm_destroy_address_space(process->address_space);
    user_process_release(process);
    return -1;
}

int user_wait(uint32_t pid)
{
    uint32_t status;
    user_process *caller;
    user_process *child;

    caller = user_process_find_by_task_id(sched_current_task_id());
    if (caller == NULL)
        return -1;

    while (1) {
        child = user_process_find_by_pid(pid);
        if (child == NULL || child->parent_pid != caller->pid)
            return -1;
        if (child->state == USER_PROCESS_EXITED)
            break;

        sched_yield();
    }

    status = child->exit_status;
    user_process_release(child);
    return (int)status;
}

static void user_exec_task(void)
{
    user_process *process;

    process = user_process_find_by_task_id(sched_current_task_id());
    if (process == NULL)
        return;

    if (user_prepare_exec(process, process->path) != 0)
        goto fail;

    if (sched_set_current_address_space(process->address_space) != 0) {
        user_process_free_pages(process);
        goto fail;
    }

    process->state = USER_PROCESS_RUNNING;
    user_enter(process);
    return;

fail:
    printk("user: failed to execute %s\n", process->path);
    if (process->address_space != 0)
        vmm_destroy_address_space(process->address_space);
    user_process_release(process);
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
    if (map_copied_user_page(process, USER_INIT_STACK_ADDR, NULL, 0,
            VMM_PAGE_WRITABLE) != 0)
        goto fail;

    process->stack = (void *)USER_INIT_STACK_ADDR;
    process->stack_size = PMM_PAGE_SIZE;

    return 0;

fail:
    user_process_free_pages(process);
    return -1;
}

uint8_t user_has_input_focus(void)
{
    user_process *process;

    if (userland_active)
        return 1;

    process = user_process_find_by_task_id(sched_current_task_id());
    return process != NULL && process->state == USER_PROCESS_RUNNING;
}

void user_exit_current(uint32_t status)
{
    user_process *parent;
    user_process *process;

    process = user_process_find_by_task_id(sched_current_task_id());
    if (process == NULL)
        return;

    process->exit_status = status;
    process->state = USER_PROCESS_EXITED;
    user_process_free_pages(process);

    // The scheduler retains the adopted directory for zombie cleanup.
    process->address_space = 0;
    user_process_orphan_children(process);

    parent = user_process_find_by_pid(process->parent_pid);
    // Keep an exited child so its parent can collect its status with wait().
    if (parent == NULL)
        user_process_release(process);
}
