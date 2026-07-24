.set ALIGN, 1<<0
.set MEMINFO, 1<<1
.set FLAGS, ALIGN | MEMINFO
.set MAGIC, 0x1BADB002
.set CHECKSUM, -(MAGIC + FLAGS)

.set KERNEL_VIRTUAL_BASE, 0xC0000000
# A page directory entry spans 4 MiB, so the higher half starts at PDE 768.
.set KERNEL_PAGE_DIRECTORY_INDEX, KERNEL_VIRTUAL_BASE >> 22
# Four page tables map the first 16 MiB during bootstrap.
.set BOOTSTRAP_PAGE_TABLES, 4
.set PAGE_ENTRIES, 1024
.set PAGE_SIZE, 4096

.section .multiboot,"a"
.align 4
.long MAGIC
.long FLAGS
.long CHECKSUM

.section .bootstrap.bss,"aw",@nobits
.align 4096
bootstrap_page_directory:
.skip PAGE_SIZE
bootstrap_page_tables:
.skip BOOTSTRAP_PAGE_TABLES * PAGE_SIZE

.section .bss
.align 16
stack_bottom:
.skip 16384
.global stack_top
stack_top:

.section .bootstrap.text,"ax"
.global _start
.type _start, @function
_start:
    cld

    # Multiboot enters with its magic in eax and info address in ebx. Preserve
    # both in callee-saved registers until the higher-half stack is active.
    mov %eax, %ebp
    mov %ebx, %esi

    # NOLOAD bootstrap storage is not assumed to be zeroed by the loader.
    mov $bootstrap_page_directory, %edi
    xor %eax, %eax
    mov $PAGE_ENTRIES, %ecx
    rep stosl

    mov $bootstrap_page_tables, %edi
    mov $0x3, %eax
    mov $(BOOTSTRAP_PAGE_TABLES * PAGE_ENTRIES), %ecx
1:
    mov %eax, (%edi)
    add $PAGE_SIZE, %eax
    add $4, %edi
    loop 1b

    mov $bootstrap_page_tables, %eax
    or $0x3, %eax
    mov $bootstrap_page_directory, %edi
    xor %ecx, %ecx
2:
    # Map the same physical frames both low and at KERNEL_VIRTUAL_BASE.
    mov %eax, (%edi, %ecx, 4)
    mov %eax, KERNEL_PAGE_DIRECTORY_INDEX * 4(%edi, %ecx, 4)
    add $PAGE_SIZE, %eax
    inc %ecx
    cmp $BOOTSTRAP_PAGE_TABLES, %ecx
    jne 2b

    # cr3 takes the physical address of the active page directory.
    mov $bootstrap_page_directory, %eax
    mov %eax, %cr3

    # Set CR0.PG. The low identity mapping keeps the next instructions valid.
    mov %cr0, %eax
    or $0x80000000, %eax
    mov %eax, %cr0

    # Normal kernel symbols are linked high and become reachable after paging.
    mov $higher_half_start, %eax
    jmp *%eax
.size _start, . - _start

.section .text
higher_half_start:
    # The final bootstrap stack is a higher-half .bss symbol.
    mov $stack_top, %esp

    call _init

    # i386 cdecl pushes arguments right-to-left: kmain(magic, mbi_addr).
    push %esi
    push %ebp
    call kmain

    cli
3:
    hlt
    jmp 3b
