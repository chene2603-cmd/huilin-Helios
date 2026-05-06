/* linker.ld */
ENTRY(_start)

SECTIONS
{
    . = 1M;                /* 内核从 1MB 开始加载 */

    .multiboot ALIGN(4) :
    {
        *(.multiboot)
    }

    .text BLOCK(4K) : ALIGN(4K)
    {
        *(.text)
    }

    .rodata BLOCK(4K) : ALIGN(4K)
    {
        *(.rodata)
    }

    .data BLOCK(4K) : ALIGN(4K)
    {
        *(.data)
    }

    .bss BLOCK(4K) : ALIGN(4K)
    {
        *(COMMON)
        *(.bss)
    }

    /DISCARD/ :
    {
        *(.comment)
        *(.note)
        *(.eh_frame)
    }
}