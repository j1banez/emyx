ENTRY(_start)

SECTIONS
{
    . = 0;

    .emxf_header : {
        BYTE(0x45) BYTE(0x4d) BYTE(0x58) BYTE(0x46)
        LONG(__emxf_code_end - __emxf_code_start)
        LONG(_start - __emxf_code_start)
    }

    .text 0x00400000 : AT(0x0c) {
        __emxf_code_start = .;
        *(.text*)
        *(.rodata*)
        __emxf_code_end = .;
    }

    /DISCARD/ : {
        *(.comment*)
        *(.eh_frame*)
        *(.note*)
    }
}
