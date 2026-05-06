#ifndef ISR_H
#define ISR_H

#include "system.h"

// 中断描述符表项
typedef struct {
    uint16_t base_low;      // 处理程序地址低16位
    uint16_t selector;      // 代码段选择子
    uint8_t zero;          // 总是0
    uint8_t flags;         // 标志位
    uint16_t base_high;     // 处理程序地址高16位
} __attribute__((packed)) idt_entry_t;

// 中断描述符表寄存器
typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) idt_ptr_t;

// 中断栈帧结构
typedef struct {
    uint32_t ds;            // 数据段
    uint32_t edi, esi, ebp, esp, ebx, edx, ecx, eax; // 通用寄存器
    uint32_t int_no, err_code; // 中断号和错误码
    uint32_t eip, cs, eflags, user_esp, ss; // 自动压栈
} registers_t;

// 函数声明
void init_idt(void);
void init_pic(void);
void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags);
void isr_handler(registers_t* regs);
void irq_handler(registers_t* regs);

// 中断处理函数类型
typedef void (*isr_t)(registers_t*);

// 注册中断处理函数
void register_interrupt_handler(uint8_t n, isr_t handler);

#endif