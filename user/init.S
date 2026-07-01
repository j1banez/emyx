.section .text

.equ SYS_WRITE, 1
.equ SYS_EXIT, 2
.equ SYS_FD_STDOUT, 1
.equ USER_DATA_SELECTOR, 0x23
.equ USER_INIT_DATA_ADDR, 0x00401000
.equ USER_INIT_MESSAGE_LEN, 21

.byte 'E', 'M', 'X', 'F'
.long init_code_end - init_code_start
.long 0

init_code_start:
    mov $USER_DATA_SELECTOR, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %fs
    mov %ax, %gs

    mov $SYS_WRITE, %eax
    mov $SYS_FD_STDOUT, %ebx
    mov $USER_INIT_DATA_ADDR, %ecx
    mov $USER_INIT_MESSAGE_LEN, %edx
    int $0x80

    mov $SYS_EXIT, %eax
    xor %ebx, %ebx
    xor %ecx, %ecx
    xor %edx, %edx
    int $0x80

1:
    jmp 1b
init_code_end:
