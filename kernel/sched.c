#include <stdint.h>
#include <stddef.h>

#include <kernel/kmalloc.h>
#include <kernel/panic.h>
#include <kernel/printk.h>
#include <kernel/sched.h>

#define MAX_TASKS 16u
#define KTHREAD_STACK_SIZE 2048u

typedef enum {
    TASK_UNUSED,
    TASK_RUNNABLE,
    TASK_RUNNING,
    TASK_ZOMBIE,
} task_state;

typedef struct {
    uint32_t id;
    task_state state;
    uintptr_t stack_pointer;
    void *stack;
    void (*entry)(void);
} task;

static task tasks[MAX_TASKS];
static uint32_t current_task;
static uint32_t next_task_id;
static uint8_t initialized;

static void kthread_trampoline(void);
static void task_cleanup(uint32_t idx);
static void reap_zombies(void);

void sched_init(void)
{
    if (initialized)
        return;

    for (uint32_t i = 0; i < MAX_TASKS; i++) {
        tasks[i].id = 0;
        tasks[i].state = TASK_UNUSED;
        tasks[i].stack_pointer = 0;
        tasks[i].entry = NULL;
        tasks[i].stack = NULL;
    }

    // Adopt the current kernel execution context as task 0.
    // Its stack pointer will be saved lazily on the first switch away.
    tasks[0].id = next_task_id++;
    tasks[0].state = TASK_RUNNING;
    current_task = 0;
    initialized = 1;
}

int kthread_create(void (*entry)(void))
{
    void *stack;

    if (entry == NULL)
        return -1;

    if (!initialized)
        sched_init();

    reap_zombies();

    for (uint32_t i = 1; i < MAX_TASKS; i++) {
        if (tasks[i].state != TASK_UNUSED)
            continue;

        stack = kmalloc(KTHREAD_STACK_SIZE);
        if (stack == NULL)
            return -1;

        tasks[i].id = next_task_id++;
        tasks[i].state = TASK_RUNNABLE;
        tasks[i].stack_pointer = sched_prepare_kthread_stack(stack,
            KTHREAD_STACK_SIZE, kthread_trampoline);
        tasks[i].stack = stack;
        tasks[i].entry = entry;

        return (int)tasks[i].id;
    }

    return -1;
}

void kthread_exit(void)
{
    if (current_task == 0)
        panic("sched: task 0 exited");

    tasks[current_task].state = TASK_ZOMBIE;
    sched_yield();
    panic("sched: zombie task resumed");
}

void sched_yield(void)
{
    uint32_t old_task;

    if (!initialized)
        sched_init();

    reap_zombies();

    // Find the first runnable task from the current one.
    for (uint32_t i = 1; i <= MAX_TASKS; i++) {
        uint32_t idx = (current_task + i) % MAX_TASKS;

        if (tasks[idx].state != TASK_RUNNABLE)
            continue;

        old_task = current_task;
        if (tasks[current_task].state == TASK_RUNNING)
            tasks[current_task].state = TASK_RUNNABLE;
        tasks[idx].state = TASK_RUNNING;
        current_task = idx;
        sched_context_switch(&tasks[old_task].stack_pointer,
            tasks[idx].stack_pointer);
        return;
    }

    printk("sched: no runnable tasks\n");
}

static void kthread_trampoline(void)
{
    tasks[current_task].entry();
    kthread_exit();
}

static void task_cleanup(uint32_t idx)
{
    tasks[idx].id = 0;
    tasks[idx].state = TASK_UNUSED;
    tasks[idx].stack_pointer = 0;
    tasks[idx].entry = NULL;

    if (tasks[idx].stack != NULL) {
        kfree(tasks[idx].stack);
        tasks[idx].stack = NULL;
    }
}

static void reap_zombies(void)
{
    for (uint32_t i = 1; i < MAX_TASKS; i++) {
        if (i == current_task || tasks[i].state != TASK_ZOMBIE)
            continue;

        task_cleanup(i);
    }
}
