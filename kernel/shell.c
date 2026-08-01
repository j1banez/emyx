#include <string.h>
#include <stdint.h>

#include <kernel/arch.h>
#include <kernel/initramfs.h>
#include <kernel/interrupt.h>
#include <kernel/keyboard.h>
#include <kernel/kmalloc.h>
#include <kernel/pmm.h>
#include <kernel/printk.h>
#include <kernel/sched.h>
#include <kernel/shell.h>
#include <kernel/timer.h>
#include <kernel/tty.h>
#include <kernel/user.h>
#include <kernel/vmm.h>

typedef struct {
    const char *name;
    const char *desc;
    void (*fn)(void);
} shell_cmd;

static void shell_exec(void);
static void cmd_help(void);
static void cmd_clear(void);
static void cmd_ticks(void);
static void cmd_irq(void);
static void cmd_panic(void);
static void cmd_reboot(void);
static void cmd_pagefault(void);
static void sched_zero_address_space_test(void);
static void cmd_vmmtest(void);
static void cmd_heaptest(void);
static void cmd_initramfs(void);
static void cmd_userspawn(void);
static void cmd_yield(void);

static char buffer[128];
static uint32_t length;
static uint8_t command_ready;

static const shell_cmd commands[] = {
    { "help", "List commands", cmd_help },
    { "clear", "Clear screen", cmd_clear },
    { "ticks", "Show timer", cmd_ticks },
    { "irq", "Show IRQ info", cmd_irq },
    { "panic", "Trigger a kernel panic", cmd_panic },
    { "reboot", "Reboot machine", cmd_reboot },
    { "pagefault", "Trigger page fault", cmd_pagefault },
    { "vmmtest", "Run VMM smoke test", cmd_vmmtest },
    { "heaptest", "Run heap smoke test", cmd_heaptest },
    { "initramfs", "Show first boot module", cmd_initramfs },
    { "userspawn", "Spawn user program by path", cmd_userspawn },
    { "yield", "Yield to the next runnable task", cmd_yield },
};

void shell_init(void)
{
    memset(buffer, 0, sizeof(buffer));
    length = 0;
    command_ready = 0;
    printk("emyx> ");
}

void shell_on_char(char c)
{
    switch (c) {
        case '\n':
            printk("%c", c);
            command_ready = 1;
            break;
        case '\b':
            if (length > 0) {
                length--;
                buffer[length] = '\0';
                terminal_backspace();
            }
            break;
        default:
            if (length < sizeof(buffer) - 1) {
                buffer[length++] = c;
                buffer[length] = '\0';
                printk("%c", c);
            }
            break;
    }
}

void shell_poll(void)
{
    if (!command_ready)
        return;

    shell_exec();
    shell_init();
}

static void shell_exec(void)
{
    size_t input_cmd_len;

    // TODO: trim buffer

    if (strlen(buffer) == 0)
        return;

    // Length of chars before first space, ignore args.
    input_cmd_len = 0;
    while (input_cmd_len < length && buffer[input_cmd_len] != ' ')
        input_cmd_len++;

    size_t n = sizeof(commands) / sizeof(commands[0]);

    for (size_t i = 0; i < n; i++) {
        shell_cmd cmd = commands[i];
        size_t cmd_len = strlen(cmd.name);

        if (cmd_len == input_cmd_len &&
                memcmp(buffer, cmd.name, cmd_len) == 0) {
            cmd.fn();
            return;
        }
    }

    printk("Unknown command\n");
}

static void cmd_help(void)
{
    printk("Available commands:\n");

    size_t n = sizeof(commands) / sizeof(commands[0]);

    for (size_t i = 0; i < n; i++) {
        printk("- %s: %s\n", commands[i].name, commands[i].desc);
    }
}

static void cmd_clear(void)
{
    terminal_init();
}

static void cmd_ticks(void)
{
    uint32_t ticks = timer_get_ticks();

    // Assumes timer is configured to 100Hz
    printk("Ticks: %u (%x)\nUptime: %us\n", ticks, ticks, ticks / 100);
}

static void cmd_irq(void)
{
    const volatile uint32_t *counts = irq_get_counts();

    for (uint32_t i = 0; i < 16; i++) {
        printk("irq%u: %u\n", i, counts[i]);
    }
}

/*
 * Trigger an exception.
 * `volatile` prevents compiler from removing the division too aggressively.
 */
static void cmd_panic(void)
{
    printk("Triggering exception...\n");

    volatile uint32_t one = 1;
    volatile uint32_t zero = 0;
    volatile uint32_t crash = one / zero;
    (void)crash;
}

static void cmd_reboot(void)
{
    printk("Rebooting...\n");
    arch_reboot();
}

static void cmd_pagefault(void)
{
    printk("Triggering pagefault...\n");

    volatile uint32_t *bad = (volatile uint32_t *)VMM_DIRECT_MAP_LIMIT;
    volatile uint32_t val = *bad;
    (void)val;
}

static void cmd_vmmtest(void)
{
    uintptr_t address_space;
    uintptr_t kernel_paddr;
    uintptr_t shared_kernel_paddr;
    uintptr_t user_page;
    uintptr_t user_paddr;
    uintptr_t paddr;
    int lookup_ret;
    int ret;
    int unmap_ret;

    ret = sched_set_current_address_space(vmm_get_kernel_address_space());
    printk("sched task0 address space: ret=%x\n", ret);

    ret = kthread_create(sched_zero_address_space_test);
    printk("sched address space test task=%x\n", ret);
    if (ret >= 0)
        sched_yield();

    address_space = vmm_create_address_space();
    kernel_paddr = 0;
    shared_kernel_paddr = 0;
    ret = vmm_get_paddr((uintptr_t)cmd_vmmtest, &kernel_paddr);
    ret |= vmm_get_paddr_in(address_space, (uintptr_t)cmd_vmmtest,
        &shared_kernel_paddr);
    if (kernel_paddr != shared_kernel_paddr)
        ret = -1;
    printk("vmm address space=%x kernel=%x shared=%x ret=%x\n",
        address_space, kernel_paddr, shared_kernel_paddr, ret);

    user_page = pmm_alloc_page();
    user_paddr = 0;
    ret = vmm_map_page_in(address_space, 0x00400000, user_page,
        VMM_PAGE_PRESENT | VMM_PAGE_WRITABLE | VMM_PAGE_USER);
    ret |= vmm_get_paddr_in(address_space, 0x00400000, &user_paddr);
    if (user_paddr != user_page)
        ret = -1;
    printk("vmm user map page=%x paddr=%x ret=%x\n",
        user_page, user_paddr, ret);

    ret = vmm_map_page_in(address_space, 0x00400000, user_page,
        VMM_PAGE_PRESENT | VMM_PAGE_WRITABLE | VMM_PAGE_USER);
    printk("vmm duplicate map: ret=%x\n", ret);

    unmap_ret = vmm_unmap_page_in(address_space, 0x00400000);
    lookup_ret = vmm_get_paddr_in(address_space, 0x00400000, &user_paddr);
    printk("vmm user unmap: ret=%x lookup=%x\n", unmap_ret, lookup_ret);
    pmm_free_page(user_page);
    vmm_destroy_address_space(address_space);

    paddr = 0;
    ret = vmm_get_paddr(0x00F00000, &paddr);
    printk("vmm kernel low before: ret=%x\n", ret);

    ret = vmm_map_page(0x00F00000, 0x00000000,
        VMM_PAGE_PRESENT | VMM_PAGE_WRITABLE);
    printk("vmm_map_page: ret=%x\n", ret);

    ret = vmm_get_paddr(0x00F00000, &paddr);
    printk("vmm_get_paddr after map: ret=%x paddr=%x\n", ret, paddr);

    ret = vmm_unmap_page(0x00F00000);
    printk("vmm_unmap_page: ret=%x\n", ret);

    ret = vmm_get_paddr(0x00F00000, &paddr);
    printk("vmm kernel low after: ret=%x\n", ret);
}

static void sched_zero_address_space_test(void)
{
    int ret;

    ret = sched_set_current_address_space(0);
    printk("sched zero address space: ret=%x\n", ret);
}

static void cmd_heaptest(void)
{
    uint32_t *a = kmalloc(sizeof(*a));
    uint32_t *b = kmalloc(sizeof(*b));

    printk("kmalloc a=%x b=%x\n", a, b);

    if (a == NULL || b == NULL)
        return;

    *a = 0x12345678;
    *b = 0x9abcdef0;

    printk("heap values a=%x b=%x\n", *a, *b);
    kfree(a);

    uint32_t *c = kmalloc(sizeof(*c));

    printk("kmalloc c=%x\n", c);

    if (c == NULL)
        return;

    *c = 0xfeedbeef;

    printk("heap value c=%x reused=%x\n", *c, c == a);

    kfree(b);
    kfree(c);
}

static void cmd_initramfs(void)
{
    initramfs_list();
}

static void cmd_userspawn(void)
{
    const char *path;
    size_t cmd_len;
    int task;

    cmd_len = strlen("userspawn");
    if (length <= cmd_len || buffer[cmd_len] != ' ') {
        printk("usage: userspawn PATH\n");
        return;
    }

    path = buffer + cmd_len + 1;
    task = user_spawn(path);
    printk("userspawn: task=%x\n", task);

    if (task < 0)
        printk("userspawn: failed to create user task\n");
}

static void cmd_yield(void)
{
    sched_init();
    keyboard_buffer_clear();
    sched_yield();
}
