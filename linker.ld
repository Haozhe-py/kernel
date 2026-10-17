ENTRY(_start)
OUTPUT_FORMAT(elf64-x86-64)

SECTIONS
{
    . = 1M;

    .text : ALIGN(4K)
    {
        KEEP(*(.text.boot))
        *(.text)
        *(.text.*)
    }

    .rodata : ALIGN(4K)
    {
        *(.rodata)
        *(.rodata.*)
    }

    .data : ALIGN(4K)
    {
        *(.data)
        *(.data.*)
    }

    .bss : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
        *(.bss.*)
    }

    /DISCARD/ :
    {
        *(.comment)
        *(.eh_frame)
        *(.note.*)
        *(.rela.*)
    }
}
