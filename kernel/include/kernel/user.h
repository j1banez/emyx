#ifndef _KERNEL_USER_H
#define _KERNEL_USER_H

#include <stdint.h>
#include <stddef.h>

#include <kernel/initramfs.h>

#define USER_INIT_CODE_ADDR 0x00400000u
#define USER_INIT_STACK_ADDR 0x007ff000u
#define USER_PROCESS_MAX_PAGES 16u
#define USER_PROCESS_MAX 16u
#define USER_PROCESS_NO_PARENT 0u
#define EMXF_HEADER_SIZE 12u
#define EMXF_MAGIC0 'E'
#define EMXF_MAGIC1 'M'
#define EMXF_MAGIC2 'X'
#define EMXF_MAGIC3 'F'

typedef struct {
    uintptr_t vaddr;
    uintptr_t paddr;
} user_page;

typedef enum {
    USER_PROCESS_FREE,
    USER_PROCESS_CREATED,
    USER_PROCESS_RUNNING,
    USER_PROCESS_EXITED,
} user_process_state;

typedef struct {
    uint32_t pid;
    uint32_t task_id;
    uint32_t parent_pid;
    uintptr_t address_space;
    uint32_t entry;
    void *stack;
    size_t stack_size;
    uint32_t exit_status;
    user_process_state state;
    char path[EMXA_PATH_SIZE];
    user_page pages[USER_PROCESS_MAX_PAGES];
    uint32_t page_count;
} user_process;

void user_init(void);
int user_spawn(const char *path);
int user_wait(uint32_t pid);
uint8_t user_has_input_focus(void);
void user_enter(user_process *process);
void user_exit_current(uint32_t status);

#endif
