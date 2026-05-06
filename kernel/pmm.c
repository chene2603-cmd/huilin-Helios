#include "system.h"
#include "screen.h"
#include "pmm.h"

// 物理内存管理器
static pmm_manager_t pmm;

// 位图操作宏
#define BITMAP_INDEX(bit) ((bit) / 32)
#define BITMAP_OFFSET(bit) ((bit) % 32)
#define BITMAP_SET(bit) (pmm.bitmap[BITMAP_INDEX(bit)] |= (1 << BITMAP_OFFSET(bit)))
#define BITMAP_CLEAR(bit) (pmm.bitmap[BITMAP_INDEX(bit)] &= ~(1 << BITMAP_OFFSET(bit)))
#define BITMAP_TEST(bit) (pmm.bitmap[BITMAP_INDEX(bit)] & (1 << BITMAP_OFFSET(bit)))

// 初始化物理内存管理器
void init_pmm(uint32_t total_memory_kb) {
    screen_putstr("Initializing Physical Memory Manager... ");
    
    // 计算总页数
    pmm.total_memory = total_memory_kb * 1024;
    pmm.total_pages = pmm.total_memory / PAGE_SIZE;
    pmm.free_pages = pmm.total_pages;
    
    // 计算位图大小
    uint32_t bitmap_size = (pmm.total_pages + 31) / 32;
    bitmap_size = (bitmap_size + 3) & ~3;  // 对齐到4字节
    
    // 位图放在内核结束后的位置
    pmm.bitmap = (uint32_t*)0x200000;  // 2MB地址
    memset(pmm.bitmap, 0, bitmap_size * 4);
    
    // 标记已使用的内存区域
    // 1. 0-1MB: BIOS、VGA、内核等
    for (uint32_t i = 0; i < 256; i++) {  // 0-1MB
        BITMAP_SET(i);
        pmm.free_pages--;
    }
    
    // 2. 位图所在区域
    uint32_t bitmap_start = (uint32_t)pmm.bitmap / PAGE_SIZE;
    uint32_t bitmap_pages = (bitmap_size * 4 + PAGE_SIZE - 1) / PAGE_SIZE;
    
    for (uint32_t i = 0; i < bitmap_pages; i++) {
        BITMAP_SET(bitmap_start + i);
        pmm.free_pages--;
    }
    
    // 3. 内核区域（假设内核在1MB-2MB之间）
    uint32_t kernel_start = 0x100000 / PAGE_SIZE;  // 1MB
    uint32_t kernel_pages = 256;  // 1MB内核
    
    for (uint32_t i = 0; i < kernel_pages; i++) {
        BITMAP_SET(kernel_start + i);
        pmm.free_pages--;
    }
    
    pmm.initialized = 1;
    
    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("OK\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr("  Total memory: ");
    screen_putdec(pmm.total_memory / 1024 / 1024);
    screen_putstr(" MB\n");
    screen_putstr("  Total pages: ");
    screen_putdec(pmm.total_pages);
    screen_putstr(" (");
    screen_putdec(pmm.total_pages * PAGE_SIZE / 1024);
    screen_putstr(" KB)\n");
    screen_putstr("  Free pages: ");
    screen_putdec(pmm.free_pages);
    screen_putstr(" (");
    screen_putdec(pmm.free_pages * PAGE_SIZE / 1024);
    screen_putstr(" KB)\n");
}

// 分配一个物理页框
void* alloc_frame(void) {
    if (!pmm.initialized) {
        screen_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
        screen_putstr("PMM not initialized!\n");
        return NULL;
    }
    
    if (pmm.free_pages == 0) {
        screen_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
        screen_putstr("Out of physical memory!\n");
        return NULL;
    }
    
    // 查找空闲页框
    for (uint32_t i = 0; i < pmm.total_pages; i++) {
        if (!BITMAP_TEST(i)) {
            BITMAP_SET(i);
            pmm.free_pages--;
            
            return (void*)(i * PAGE_SIZE);
        }
    }
    
    return NULL;
}

// 释放一个物理页框
void free_frame(void* frame) {
    if (!pmm.initialized || !frame) {
        return;
    }
    
    uint32_t index = (uint32_t)frame / PAGE_SIZE;
    
    if (index >= pmm.total_pages) {
        screen_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
        screen_putstr("Invalid frame to free: ");
        screen_puthex((uint32_t)frame);
        screen_putstr("\n");
        return;
    }
    
    if (!BITMAP_TEST(index)) {
        screen_setcolor(COLOR_YELLOW, COLOR_BLACK);
        screen_putstr("Warning: Double free of frame ");
        screen_puthex((uint32_t)frame);
        screen_putstr("\n");
        return;
    }
    
    BITMAP_CLEAR(index);
    pmm.free_pages++;
}

// 分配连续物理页框
void* alloc_frames(uint32_t count) {
    if (count == 0 || count > pmm.free_pages) {
        return NULL;
    }
    
    // 简单实现：分配不保证连续
    // 实际操作系统需要实现伙伴系统
    void* frame = alloc_frame();
    if (!frame) {
        return NULL;
    }
    
    return frame;
}

// 获取物理内存信息
void get_pmm_info(pmm_info_t* info) {
    if (!info) return;
    
    info->total_memory = pmm.total_memory;
    info->total_pages = pmm.total_pages;
    info->free_pages = pmm.free_pages;
    info->used_pages = pmm.total_pages - pmm.free_pages;
}

// 显示内存状态
void show_memory_status(void) {
    screen_setcolor(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_putstr("\nPhysical Memory Status:\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr("  Total: ");
    screen_putdec(pmm.total_memory / 1024);
    screen_putstr(" KB (");
    screen_putdec(pmm.total_memory / 1024 / 1024);
    screen_putstr(" MB)\n");
    screen_putstr("  Free:  ");
    screen_putdec(pmm.free_pages * PAGE_SIZE / 1024);
    screen_putstr(" KB (");
    screen_putdec(pmm.free_pages);
    screen_putstr(" pages)\n");
    screen_putstr("  Used:  ");
    screen_putdec((pmm.total_pages - pmm.free_pages) * PAGE_SIZE / 1024);
    screen_putstr(" KB\n");
    
    // 显示位图片段
    screen_putstr("\nMemory bitmap (first 32 pages): ");
    for (int i = 0; i < 32; i++) {
        if (i % 8 == 0) screen_putstr(" ");
        screen_putchar(BITMAP_TEST(i) ? 'X' : '.');
    }
    screen_putstr("\n");
}