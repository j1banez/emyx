#include <string.h>
#include <stdint.h>

#include <kernel/arch.h>
#include <kernel/interrupt.h>
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
static void cmd_vmmtest(void);
static void cmd_heaptest(void);
static void cmd_userinit(void);
static void cmd_newtask(void);
static void cmd_yield(void);
static void test_task(void);

static char buffer[128];
static uint32_t length;

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
    { "userinit", "Run embedded user init program", cmd_userinit },
    { "newtask", "Create one scheduler test task", cmd_newtask },
    { "yield", "Yield to the next runnable task", cmd_yield },
};

void shell_init(void)
{
    memset(buffer, 0, sizeof(buffer));
    length = 0;
    printk("emyx> ");
}

void shell_on_char(char c)
{
    switch (c) {
        case '\n':
            printk("%c", c);
            shell_exec();
            shell_init();
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

static void shell_exec(void)
{
    // TODO: trim buffer

    if (strlen(buffer) == 0)
        return;

    size_t n = sizeof(commands) / sizeof(commands[0]);

    for (size_t i = 0; i < n; i++) {
        shell_cmd cmd = commands[i];
        size_t cmd_len = strlen(cmd.name);

        if (cmd_len == length && memcmp(buffer, cmd.name, cmd_len) == 0) {
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

    volatile uint32_t *bad = (volatile uint32_t *)VMM_BOOTSTRAP_LIMIT;
    volatile uint32_t val = *bad;
    (void)val;
}

static void cmd_vmmtest(void)
{
    uintptr_t paddr;
    uintptr_t original_paddr;
    int ret;

    ret = vmm_get_physaddr(0x00F00000, &paddr);
    printk("vmm_get_physaddr before: ret=%x paddr=%x\n", ret, paddr);

    if (ret != 0)
        return;

    original_paddr = paddr & ~(uintptr_t)(PMM_PAGE_SIZE - 1);

    ret = vmm_map_page(0x00F00000, 0x00000000,
        VMM_PAGE_PRESENT | VMM_PAGE_WRITABLE);
    printk("vmm_map_page: ret=%x\n", ret);

    ret = vmm_get_physaddr(0x00F00000, &paddr);
    printk("vmm_get_physaddr after map: ret=%x paddr=%x\n", ret, paddr);

    ret = vmm_unmap_page(0x00F00000);
    printk("vmm_unmap_page: ret=%x\n", ret);

    ret = vmm_map_page(0x00F00000, original_paddr,
        VMM_PAGE_PRESENT | VMM_PAGE_WRITABLE);
    printk("vmm_restore_page: ret=%x\n", ret);

    ret = vmm_get_physaddr(0x00F00000, &paddr);
    printk("vmm_get_physaddr after restore: ret=%x paddr=%x\n", ret, paddr);
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

static void cmd_userinit(void)
{
    printk("Running embedded user init...\n");
    user_run_init();
}

static void cmd_newtask(void)
{
    int task;

    sched_init();

    task = kthread_create(test_task);
    printk("newtask: task=%x\n", task);

    if (task < 0) {
        printk("newtask: failed to create test task\n");
        return;
    }
}

static void cmd_yield(void)
{
    sched_init();
    sched_yield();
}

static void test_task(void)
{
    printk("newtask: task ran\n");
}
