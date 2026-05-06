#ifndef SCREEN_H
#define SCREEN_H

#include "system.h"

// VGA文本模式相关常量
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

// 颜色定义
enum vga_color {
    COLOR_BLACK = 0,
    COLOR_BLUE = 1,
    COLOR_GREEN = 2,
    COLOR_CYAN = 3,
    COLOR_RED = 4,
    COLOR_MAGENTA = 5,
    COLOR_BROWN = 6,
    COLOR_LIGHT_GREY = 7,
    COLOR_DARK_GREY = 8,
    COLOR_LIGHT_BLUE = 9,
    COLOR_LIGHT_GREEN = 10,
    COLOR_LIGHT_CYAN = 11,
    COLOR_LIGHT_RED = 12,
    COLOR_LIGHT_MAGENTA = 13,
    COLOR_LIGHT_BROWN = 14,
    COLOR_WHITE = 15,
};

// 屏幕结构
typedef struct {
    uint16_t* buffer;
    uint8_t color;
    uint8_t cursor_x;
    uint8_t cursor_y;
} screen_t;

// 函数声明
void screen_init(void);
void screen_clear(void);
void screen_setcolor(uint8_t fg, uint8_t bg);
void screen_putchar(char c);
void screen_putstr(const char* str);
void screen_puthex(uint32_t n);
void screen_putdec(uint32_t n);
void screen_scroll(void);
void screen_set_cursor(uint8_t x, uint8_t y);
void screen_get_cursor(uint8_t* x, uint8_t* y);

#endif