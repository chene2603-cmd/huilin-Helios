/* 链接器脚本 */
OUTPUT_FORMAT(elf32-i386)
ENTRY(_start)

SECTIONS
{
    /* 内核加载地址 */
    . = 0x100000;
    
    /* 多引导头 */
    .multiboot : {
        *(.multiboot)
    }
    
    /* 代码段 */
    .text : {
        *(.text)
    }
    
    /* 只读数据段 */
    .rodata : {
        *(.rodata)
    }
    
    /* 数据段 */
    .data : {
        *(.data)
    }
    
    /* BSS段 */
    .bss : {
        *(COMMON)
        *(.bss)
    }
    
    /* 结束标记 */
    . = ALIGN(4096);
    _end = .;
}