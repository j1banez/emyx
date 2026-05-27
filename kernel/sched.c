#include <stdint.h>
#include <stddef.h>

#include <kernel/kmalloc.h>
#include <kernel/printk.h>
#include <kernel/sched.h>

#define MAX_TASKS 16u
#define KTHREAD_STACK_SIZE 2048u

typedef enum {
    TASK_UNUSED,
    TASK_RUNNABLE,
    TASK_RUNNING,
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

    for (uint32_t i = 1; i < MAX_TASKS; i++) {
        if (tasks[i].state != TASK_UNUSED)
            continue;

        stack = kmalloc(KTHREAD_STACK_SIZE);
        if (stack == NULL)
            return -1;

        tasks[i].id = next_task_id++;
        tasks[i].state = TASK_RUNNABLE;
        tasks[i].stack_pointer = (uintptr_t)stack + KTHREAD_STACK_SIZE;
        tasks[i].stack = stack;
        tasks[i].entry = entry;

        printk("sched: created task id=%u slot=%u stack=%x\n",
            tasks[i].id, i, stack);
        return (int)tasks[i].id;
    }

    return -1;
}

void sched_yield(void)
{
    if (!initialized)
        sched_init();

    for (uint32_t i = 1; i <= MAX_TASKS; i++) {
        uint32_t idx = (current_task + i) % MAX_TASKS;

        if (tasks[idx].state != TASK_RUNNABLE)
            continue;

        printk("sched: switch current=%u to next=%u\n",
            tasks[current_task].id, tasks[idx].id);

        tasks[current_task].state = TASK_RUNNABLE;
        tasks[idx].state = TASK_RUNNING;
        current_task = idx;
        return;
    }

    printk("sched: no runnable tasks\n");
}
