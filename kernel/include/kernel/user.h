#ifndef _KERNEL_USER_H
#define _KERNEL_USER_H

#include <stdint.h>
#include <stddef.h>

#define USER_INIT_CODE_ADDR 0x00400000u
#define USER_INIT_DATA_ADDR 0x00401000u
#define USER_INIT_STACK_TOP 0x00800000u
#define USER_INIT_MESSAGE_LEN 21u
#define USER_PROCESS_MAX_PAGES 16u
#define EMXF_HEADER_SIZE 12u
#define EMXF_MAGIC0 'E'
#define EMXF_MAGIC1 'M'
#define EMXF_MAGIC2 'X'
#define EMXF_MAGIC3 'F'

typedef struct {
    uintptr_t vaddr;
    uintptr_t paddr;
} user_page;

typedef struct {
    uint32_t id;
    uint32_t entry;
    uint32_t stack_top;
    uint32_t exit_status;
    uint8_t exited;
    user_page pages[USER_PROCESS_MAX_PAGES];
    uint32_t page_count;
} user_process;

void user_init(void);
user_process *user_process_create(void);
user_process *user_current_process(void);
int user_prepare_init(user_process *process);
void user_enter(user_process *process);
void user_exit_current(uint32_t status);

#endif
