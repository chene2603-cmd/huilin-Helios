; kernel/isr.asm
; 中断服务例程，保存寄存器，调用 C 处理函数，恢复寄存器并 iret

%macro ISR_NOERR 1
global isr%1
isr%1:
    push 0               ; 压入假错误码
    push %1              ; 压入中断号
    jmp common_isr
%endmacro

%macro ISR_ERR 1
global isr%1
isr%1:
    push %1              ; 压入中断号（错误码已由CPU压入）
    jmp common_isr
%endmacro

%macro IRQ 2
global irq%1
irq%1:
    push 0
    push %2
    jmp common_irq
%endmacro

section .text
extern isr_handler
extern irq_handler

common_isr:
    pusha                ; 保存通用寄存器

    mov ax, ds
    push eax             ; 保存数据段

    mov ax, 0x10         ; 加载内核数据段选择子
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call isr_handler

    pop eax              ; 恢复段寄存器
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                 ; 恢复通用寄存器
    add esp, 8           ; 清理中断号和错误码
    iret

common_irq:
    pusha

    mov ax, ds
    push eax

    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call irq_handler

    pop eax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa
    add esp, 8
    iret

; 定义 ISR 0~31
ISR_NOERR  0
ISR_NOERR  1
ISR_NOERR  2
ISR_NOERR  3
ISR_NOERR  4
ISR_NOERR  5
ISR_NOERR  6
ISR_NOERR  7
ISR_ERR    8
ISR_NOERR  9
ISR_ERR   10
ISR_ERR   11
ISR_ERR   12
ISR_ERR   13
ISR_ERR   14
ISR_NOERR 15
ISR_NOERR 16
ISR_NOERR 17
ISR_NOERR 18
ISR_NOERR 19
ISR_NOERR 20
ISR_NOERR 21
ISR_NOERR 22
ISR_NOERR 23
ISR_NOERR 24
ISR_NOERR 25
ISR_NOERR 26
ISR_NOERR 27
ISR_NOERR 28
ISR_NOERR 29
ISR_NOERR 30
ISR_NOERR 31

; 定义 IRQ 0~15（映射到 32~47）
IRQ  0, 32
IRQ  1, 33
IRQ  2, 34
IRQ  3, 35
IRQ  4, 36
IRQ  5, 37
IRQ  6, 38
IRQ  7, 39
IRQ  8, 40
IRQ  9, 41
IRQ 10, 42
IRQ 11, 43
IRQ 12, 44
IRQ 13, 45
IRQ 14, 46
IRQ 15, 47