#include "screen.h"

static screen_t screen;

// 创建颜色字节
static inline uint8_t make_color(enum vga_color fg, enum vga_color bg) {
    return fg | bg << 4;
}

// 创建VGA条目
static inline uint16_t make_vgaentry(char c, uint8_t color) {
    uint16_t c16 = c;
    uint16_t color16 = color;
    return c16 | color16 << 8;
}

// 初始化屏幕
void screen_init(void) {
    screen.buffer = (uint16_t*)VGA_MEMORY;
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_clear();
}

// 清屏
void screen_clear(void) {
    uint16_t blank = make_vgaentry(' ', screen.color);
    
    for (uint32_t i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        screen.buffer[i] = blank;
    }
    
    screen.cursor_x = 0;
    screen.cursor_y = 0;
    screen_set_cursor(0, 0);
}

// 设置颜色
void screen_setcolor(uint8_t fg, uint8_t bg) {
    screen.color = make_color(fg, bg);
}

// 显示字符
void screen_putchar(char c) {
    // 处理特殊字符
    if (c == '\n') {
        screen.cursor_x = 0;
        screen.cursor_y++;
    } else if (c == '\r') {
        screen.cursor_x = 0;
    } else if (c == '\t') {
        screen.cursor_x = (screen.cursor_x + 8) & ~(8 - 1);
    } else if (c == '\b') {
        if (screen.cursor_x > 0) {
            screen.cursor_x--;
        }
    } else {
        uint16_t entry = make_vgaentry(c, screen.color);
        uint32_t index = screen.cursor_y * VGA_WIDTH + screen.cursor_x;
        screen.buffer[index] = entry;
        screen.cursor_x++;
    }
    
    // 处理换行和滚屏
    if (screen.cursor_x >= VGA_WIDTH) {
        screen.cursor_x = 0;
        screen.cursor_y++;
    }
    
    if (screen.cursor_y >= VGA_HEIGHT) {
        screen_scroll();
        screen.cursor_y = VGA_HEIGHT - 1;
    }
    
    // 更新光标位置
    screen_set_cursor(screen.cursor_x, screen.cursor_y);
}

// 显示字符串
void screen_putstr(const char* str) {
    while (*str) {
        screen_putchar(*str++);
    }
}

// 显示十六进制数
void screen_puthex(uint32_t n) {
    screen_putstr("0x");
    
    int shift = 28;
    while (shift >= 0) {
        uint8_t digit = (n >> shift) & 0xF;
        if (digit < 10) {
            screen_putchar('0' + digit);
        } else {
            screen_putchar('A' + digit - 10);
        }
        shift -= 4;
    }
}

// 显示十进制数
void screen_putdec(uint32_t n) {
    if (n == 0) {
        screen_putchar('0');
        return;
    }
    
    char buffer[12];
    int i = 0;
    
    while (n > 0) {
        buffer[i++] = '0' + (n % 10);
        n /= 10;
    }
    
    // 反转字符串
    for (int j = i - 1; j >= 0; j--) {
        screen_putchar(buffer[j]);
    }
}

// 滚屏
void screen_scroll(void) {
    uint16_t blank = make_vgaentry(' ', screen.color);
    
    // 向上移动一行
    for (uint32_t y = 0; y < VGA_HEIGHT - 1; y++) {
        for (uint32_t x = 0; x < VGA_WIDTH; x++) {
            uint32_t src_index = (y + 1) * VGA_WIDTH + x;
            uint32_t dst_index = y * VGA_WIDTH + x;
            screen.buffer[dst_index] = screen.buffer[src_index];
        }
    }
    
    // 清空最后一行
    for (uint32_t x = 0; x < VGA_WIDTH; x++) {
        screen.buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = blank;
    }
}

// 设置光标位置
void screen_set_cursor(uint8_t x, uint8_t y) {
    uint16_t pos = y * VGA_WIDTH + x;
    
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
    
    screen.cursor_x = x;
    screen.cursor_y = y;
}

// 获取光标位置
void screen_get_cursor(uint8_t* x, uint8_t* y) {
    if (x) *x = screen.cursor_x;
    if (y) *y = screen.cursor_y;
}