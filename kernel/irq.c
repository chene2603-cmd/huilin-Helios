#include "system.h"
#include "screen.h"

// PIC端口定义
#define PIC1_CMD    0x20
#define PIC1_DATA   0x21
#define PIC2_CMD    0xA0
#define PIC2_DATA   0xA1

// PIC命令
#define PIC_EOI     0x20  // 中断结束命令

// PIC初始化命令
#define PIC_INIT    0x11  // 初始化命令
#define PIC_8086    0x01  // 8086模式

// 中断向量偏移
#define PIC1_OFFSET 0x20
#define PIC2_OFFSET 0x28

// 初始化PIC
void init_pic(void) {
    // 保存掩码
    uint8_t a1 = inb(PIC1_DATA);
    uint8_t a2 = inb(PIC2_DATA);
    
    // 初始化主PIC
    outb(PIC1_CMD, PIC_INIT);
    io_wait();
    outb(PIC1_DATA, PIC1_OFFSET);
    io_wait();
    outb(PIC1_DATA, 0x04);  // 主PIC的IRQ2连接从PIC
    io_wait();
    outb(PIC1_DATA, PIC_8086);
    io_wait();
    
    // 初始化从PIC
    outb(PIC2_CMD, PIC_INIT);
    io_wait();
    outb(PIC2_DATA, PIC2_OFFSET);
    io_wait();
    outb(PIC2_DATA, 0x02);  // 从PIC连接主PIC的IRQ2
    io_wait();
    outb(PIC2_DATA, PIC_8086);
    io_wait();
    
    // 恢复掩码
    outb(PIC1_DATA, a1);
    outb(PIC2_DATA, a2);
    
    // 启用所有中断
    outb(PIC1_DATA, 0x00);
    outb(PIC2_DATA, 0x00);
}

// 发送EOI
void send_eoi(uint8_t irq) {
    if (irq >= 8) {
        outb(PIC2_CMD, PIC_EOI);
    }
    outb(PIC1_CMD, PIC_EOI);
}

// 启用IRQ
void enable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) & ~(1 << irq);
    outb(port, value);
}

// 禁用IRQ
void disable_irq(uint8_t irq) {
    uint16_t port;
    uint8_t value;
    
    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }
    
    value = inb(port) | (1 << irq);
    outb(port, value);
}