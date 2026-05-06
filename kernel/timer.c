#include "system.h"
#include "screen.h"
#include "timer.h"
#include "isr.h"

// 定时器相关
static uint32_t timer_ticks = 0;
static uint32_t timer_frequency = 100;  // 100Hz

// 初始化定时器
void init_timer(uint32_t frequency) {
    screen_putstr("Initializing Timer... ");
    
    timer_frequency = frequency;
    
    // 设置PIT（可编程间隔定时器）
    uint32_t divisor = 1193180 / frequency;
    
    // 发送命令字节
    outb(0x43, 0x36);
    
    // 发送分频器
    outb(0x40, divisor & 0xFF);
    outb(0x40, (divisor >> 8) & 0xFF);
    
    // 注册中断处理程序
    register_interrupt_handler(32, timer_interrupt_handler);
    
    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("OK\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr("  Frequency: ");
    screen_putdec(frequency);
    screen_putstr(" Hz\n");
}

// 定时器中断处理程序
void timer_interrupt_handler(registers_t* regs) {
    timer_ticks++;
    
    // 处理定时器事件
    timer_tick_handler(regs);
}

// 获取当前滴答数
uint32_t get_ticks(void) {
    return timer_ticks;
}

// 获取当前时间（毫秒）
uint32_t get_time_ms(void) {
    return timer_ticks * (1000 / timer_frequency);
}

// 更新滴答数
void update_ticks(void) {
    timer_ticks++;
}

// 延时函数（忙等待）
void delay_ms(uint32_t milliseconds) {
    uint32_t start = get_time_ms();
    while (get_time_ms() - start < milliseconds) {
        asm volatile("pause");
    }
}

// 显示时间
void show_time(void) {
    uint32_t seconds = get_time_ms() / 1000;
    uint32_t minutes = seconds / 60;
    uint32_t hours = minutes / 60;
    
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr("System uptime: ");
    if (hours > 0) {
        screen_putdec(hours);
        screen_putstr("h ");
    }
    if (minutes > 0) {
        screen_putdec(minutes % 60);
        screen_putstr("m ");
    }
    screen_putdec(seconds % 60);
    screen_putstr(".");
    
    uint32_t ms = get_time_ms() % 1000;
    if (ms < 100) screen_putstr("0");
    if (ms < 10) screen_putstr("0");
    screen_putdec(ms);
    screen_putstr("s\n");
}