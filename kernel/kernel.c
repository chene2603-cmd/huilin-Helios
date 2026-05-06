#include "system.h"
#include "screen.h"
#include "isr.h"

// 内核版本信息
#define KERNEL_NAME "Helios"
#define KERNEL_VERSION "0.1.0"
#define KERNEL_BUILD_DATE __DATE__
#define KERNEL_BUILD_TIME __TIME__

// 内存映射结构
typedef struct {
    uint32_t base_low;
    uint32_t base_high;
    uint32_t length_low;
    uint32_t length_high;
    uint32_t type;
    uint32_t acpi;
} __attribute__((packed)) memory_map_entry_t;

// 多引导信息结构
typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint32_t vbe_mode;
    uint32_t vbe_interface_seg;
    uint32_t vbe_interface_off;
    uint32_t vbe_interface_len;
} multiboot_info_t;

// 全局变量
static multiboot_info_t* mb_info = NULL;
static uint32_t total_memory = 0;

// 显示启动信息
void show_banner(void) {
    screen_setcolor(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_putstr("\n");
    screen_putstr("  _    _      _ _           \n");
    screen_putstr(" | |  | |    | (_)          \n");
    screen_putstr(" | |__| | ___| |___  __  __ \n");
    screen_putstr(" |  __  |/ _ \\ | \\ \\/ / \n");
    screen_putstr(" | |  | |  __/ | |>  <      \n");
    screen_putstr(" |_|  |_|\\___|_|_/_/\\_\\ \n");
    screen_putstr("\n");
    
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr(KERNEL_NAME " Kernel ");
    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("v" KERNEL_VERSION "\n");
    
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr("Build: " KERNEL_BUILD_DATE " " KERNEL_BUILD_TIME "\n");
    screen_putstr("Architecture: i386 Protected Mode\n\n");
}

// 检测内存
void detect_memory(void) {
    if (!(mb_info->flags & 0x01)) {
        screen_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
        screen_putstr("ERROR: No valid memory info from bootloader\n");
        return;
    }
    
    total_memory = mb_info->mem_upper + mb_info->mem_lower;
    
    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("Memory: ");
    screen_setcolor(COLOR_WHITE, COLOR_BLACK);
    screen_putdec(total_memory);
    screen_putstr(" KB (");
    screen_putdec(total_memory / 1024);
    screen_putstr(" MB)\n");
    
    // 显示内存映射
    if (mb_info->flags & 0x40) {
        memory_map_entry_t* mmap = (memory_map_entry_t*)mb_info->mmap_addr;
        memory_map_entry_t* mmap_end = (memory_map_entry_t*)(mb_info->mmap_addr + mb_info->mmap_length);
        
        screen_putstr("Memory map:\n");
        
        while (mmap < mmap_end) {
            uint64_t base = (uint64_t)mmap->base_low | ((uint64_t)mmap->base_high << 32);
            uint64_t length = (uint64_t)mmap->length_low | ((uint64_t)mmap->length_high << 32);
            
            screen_putstr("  ");
            screen_puthex((uint32_t)(base >> 32));
            screen_puthex((uint32_t)base);
            screen_putstr(" - ");
            screen_puthex((uint32_t)((base + length) >> 32));
            screen_puthex((uint32_t)(base + length));
            screen_putstr(" (");
            screen_putdec((uint32_t)(length / 1024));
            screen_putstr(" KB) ");
            
            switch (mmap->type) {
                case 1:
                    screen_putstr("Available");
                    break;
                case 2:
                    screen_putstr("Reserved");
                    break;
                case 3:
                    screen_putstr("ACPI Reclaimable");
                    break;
                case 4:
                    screen_putstr("ACPI NVS");
                    break;
                case 5:
                    screen_putstr("Bad Memory");
                    break;
                default:
                    screen_putstr("Unknown");
            }
            
            screen_putstr("\n");
            
            mmap = (memory_map_entry_t*)((uint32_t)mmap + mmap->size + sizeof(uint32_t));
        }
    }
}

// 初始化系统
void init_system(void) {
    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("\nInitializing system...\n");
    
    // 初始化中断描述符表
    screen_putstr("  Setting up IDT... ");
    init_idt();
    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("OK\n");
    
    // 初始化中断控制器
    screen_putstr("  Setting up PIC... ");
    init_pic();
    screen_putstr("OK\n");
    
    // 启用中断
    screen_putstr("  Enabling interrupts... ");
    asm volatile("sti");
    screen_putstr("OK\n");
    
    screen_setcolor(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_putstr("\nSystem initialized successfully!\n");
}

// 测试中断
void test_interrupts(void) {
    screen_setcolor(COLOR_WHITE, COLOR_BLACK);
    screen_putstr("\nTesting interrupts...\n");
    
    // 触发一个软件中断
    screen_putstr("  Triggering software interrupt 0x80... ");
    asm volatile("int $0x80");
    screen_putstr("OK\n");
    
    // 触发除零异常
    screen_putstr("  Testing divide by zero... ");
    asm volatile(
        "xorl %%eax, %%eax\n"
        "divl %%eax"
        : : : "eax", "edx");
    
    screen_putstr("Recovered from exception\n");
}

// 内核主函数
void kernel_main(multiboot_info_t* mbi, uint32_t magic) {
    // 初始化屏幕
    screen_init();
    screen_clear();
    
    // 检查多引导魔数
    if (magic != 0x2BADB002) {
        screen_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
        screen_putstr("ERROR: Invalid multiboot magic number: ");
        screen_puthex(magic);
        screen_putstr("\n");
        return;
    }
    
    mb_info = mbi;
    
    // 显示启动信息
    show_banner();
    
    // 检测内存
    detect_memory();
    
    // 初始化系统
    init_system();
    
    // 测试中断
    test_interrupts();
    
    // 进入内核主循环
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr("\n> ");
    
    // 简单命令行循环
    char input[256];
    uint32_t pos = 0;
    
    while (1) {
        // 等待键盘输入
        uint8_t key = inb(0x60);
        
        if (key < 0x80) {  // 键按下
            if (key == 0x1C) {  // 回车键
                screen_putchar('\n');
                input[pos] = '\0';
                
                // 处理命令
                if (strcmp(input, "help") == 0) {
                    screen_putstr("Available commands: help, mem, clear, echo\n");
                } else if (strcmp(input, "mem") == 0) {
                    screen_putstr("Total memory: ");
                    screen_putdec(total_memory);
                    screen_putstr(" KB\n");
                } else if (strcmp(input, "clear") == 0) {
                    screen_clear();
                    screen_putstr("> ");
                } else if (strncmp(input, "echo ", 5) == 0) {
                    screen_putstr(&input[5]);
                    screen_putstr("\n");
                } else if (input[0] != '\0') {
                    screen_putstr("Unknown command: ");
                    screen_putstr(input);
                    screen_putstr("\n");
                }
                
                screen_putstr("> ");
                pos = 0;
            } else if (key == 0x0E) {  // 退格键
                if (pos > 0) {
                    pos--;
                    screen_putchar('\b');
                }
            } else {
                // 简单键码转换
                char c = 0;
                switch (key) {
                    case 0x02: c = '1'; break;
                    case 0x03: c = '2'; break;
                    case 0x04: c = '3'; break;
                    case 0x05: c = '4'; break;
                    case 0x06: c = '5'; break;
                    case 0x07: c = '6'; break;
                    case 0x08: c = '7'; break;
                    case 0x09: c = '8'; break;
                    case 0x0A: c = '9'; break;
                    case 0x0B: c = '0'; break;
                    case 0x10: c = 'q'; break;
                    case 0x11: c = 'w'; break;
                    case 0x12: c = 'e'; break;
                    case 0x13: c = 'r'; break;
                    case 0x14: c = 't'; break;
                    case 0x15: c = 'y'; break;
                    case 0x16: c = 'u'; break;
                    case 0x17: c = 'i'; break;
                    case 0x18: c = 'o'; break;
                    case 0x19: c = 'p'; break;
                    case 0x1E: c = 'a'; break;
                    case 0x1F: c = 's'; break;
                    case 0x20: c = 'd'; break;
                    case 0x21: c = 'f'; break;
                    case 0x22: c = 'g'; break;
                    case 0x23: c = 'h'; break;
                    case 0x24: c = 'j'; break;
                    case 0x25: c = 'k'; break;
                    case 0x26: c = 'l'; break;
                    case 0x2C: c = 'z'; break;
                    case 0x2D: c = 'x'; break;
                    case 0x2E: c = 'c'; break;
                    case 0x2F: c = 'v'; break;
                    case 0x30: c = 'b'; break;
                    case 0x31: c = 'n'; break;
                    case 0x32: c = 'm'; break;
                    case 0x39: c = ' '; break;
                    case 0x1C: c = '\n'; break;
                }
                
                if (c && pos < sizeof(input) - 1) {
                    input[pos++] = c;
                    screen_putchar(c);
                }
            }
        }
        
        // 简单延时
        for (volatile int i = 0; i < 10000; i++);
    }
}