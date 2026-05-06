/* user.ld */
ENTRY(_start)

SECTIONS
{
    . = 0x400000;    /* 用户代码从 4MB 开始 */
    .text : { *(.text) }
    .data : { *(.data) }
    .bss  : { *(.bss)  }
}