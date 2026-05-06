; boot/boot.asm
; Multiboot 引导，设置基本环境，跳入内核

MBALIGN  equ 1<<0
MEMINFO  equ 1<<1
FLAGS    equ MBALIGN | MEMINFO
MAGIC    equ 0x1BADB002
CHECKSUM equ -(MAGIC + FLAGS)

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384          ; 16 KB 内核栈
stack_top:

section .text
global _start
extern kernel_main      ; 在 kernel/main.c 中定义

_start:
    mov esp, stack_top   ; 设置栈指针

    push eax             ; 传递 Multiboot 魔数
    push ebx             ; 传递 Multiboot 信息结构指针

    call kernel_main

    cli
.hang:
    hlt
    jmp .hang