ENTRY(_start)

KERNEL_VIRTUAL_BASE = 0xC0000000;

SECTIONS
{
    . = 1M;
    __kernel_physical_start = .;

    .bootstrap BLOCK(4K) : ALIGN(4K)
    {
        KEEP(*(.multiboot))
        *(.bootstrap.text)
        *(.bootstrap.data)
    }

    .bootstrap_bss BLOCK(4K) (NOLOAD) : ALIGN(4K)
    {
        *(.bootstrap.bss)
    }

    . += KERNEL_VIRTUAL_BASE;
    __kernel_start = .;

    .text BLOCK(4K) : AT(ADDR(.text) - KERNEL_VIRTUAL_BASE)
    {
        *(.text)
    }

    .rodata BLOCK(4K) : AT(ADDR(.rodata) - KERNEL_VIRTUAL_BASE)
    {
        *(.rodata)
    }

    .data BLOCK(4K) : AT(ADDR(.data) - KERNEL_VIRTUAL_BASE)
    {
        *(.data)
    }

    .bss BLOCK(4K) : AT(ADDR(.bss) - KERNEL_VIRTUAL_BASE)
    {
        *(COMMON)
        *(.bss)
    }

    __kernel_end = .;
    __kernel_physical_end = __kernel_end - KERNEL_VIRTUAL_BASE;
}
