#include "system.h"
#include "timer.h"
#include "isr.h"
#include "screen.h"

// PIT 频率（Hz），可配置
#define PIT_BASE_FREQUENCY 1193180

static volatile uint32_t tick_count = 0;        // 滴答数（从启动开始）
static uint32_t frequency = 100;                // 默认 100 Hz
static uint32_t ms_per_tick = 10;               // 每滴答毫秒数

// 定时器中断处理程序（IRQ0）
static void timer_callback(registers_t *regs) {
    (void)regs;
    tick_count++;
    // 这里可以添加唤醒睡眠进程的逻辑，当前版本先留空
}

// 初始化定时器
void init_timer(uint32_t freq) {
    if (freq == 0) freq = 100;
    frequency = freq;

    // 计算 PIT 分频值
    uint32_t divisor = PIT_BASE_FREQUENCY / freq;

    // 计算每滴答的毫秒数（近似，可能稍有误差）
    ms_per_tick = 1000 / freq;

    // 设置 PIT 通道 0 为模式 3（方波发生器）
    outb(0x43, 0x36);                     // 命令字节：通道0、低字节/高字节、模式3
    outb(0x40, (uint8_t)(divisor & 0xFF));
    outb(0x40, (uint8_t)((divisor >> 8) & 0xFF));

    // 注册 IRQ0 处理函数（IRQ0 对应中断向量 32）
    register_interrupt_handler(32, timer_callback);

    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("Timer initialized: ");
    screen_putdec(freq);
    screen_putstr(" Hz\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
}

// 获取系统启动以来的毫秒数
uint32_t get_time_ms(void) {
    return tick_count * ms_per_tick;
}

// 获取滴答计数
uint32_t get_ticks(void) {
    return tick_count;
}

// 忙等待（毫秒级，简单实现）
void delay_ms(uint32_t ms) {
    uint32_t start = tick_count * ms_per_tick;
    while ((tick_count * ms_per_tick) - start < ms) {
        asm volatile("hlt");   // 省电等待
    }
}