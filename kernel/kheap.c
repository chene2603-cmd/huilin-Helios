#include "system.h"
#include "screen.h"
#include "kheap.h"
#include "paging.h"

// 堆管理器
static heap_t kernel_heap;

// 内存块头部
typedef struct block_header {
    uint32_t size;           // 块大小（包括头部）
    uint8_t is_free;         // 是否空闲
    struct block_header* next;  // 下一个块
    struct block_header* prev;  // 前一个块
} block_header_t;

// 初始化内核堆
void init_kheap(uint32_t start, uint32_t size) {
    screen_putstr("Initializing Kernel Heap... ");
    
    kernel_heap.start = start;
    kernel_heap.size = size;
    kernel_heap.used = sizeof(block_header_t);
    
    // 初始化第一个空闲块
    block_header_t* first_block = (block_header_t*)start;
    first_block->size = size;
    first_block->is_free = 1;
    first_block->next = NULL;
    first_block->prev = NULL;
    
    // 启用分页区域的堆
    kernel_heap.start = 0xD0000000;  // 3.25GB
    kernel_heap.size = 0x10000000;   // 256MB
    
    // 映射虚拟地址到物理内存
    for (uint32_t i = 0; i < kernel_heap.size; i += PAGE_SIZE) {
        void* phys = alloc_frame();
        if (phys) {
            map_page(get_kernel_page_dir(), 
                    (void*)(kernel_heap.start + i),
                    phys,
                    PAGE_PRESENT | PAGE_WRITE);
        }
    }
    
    // 重新初始化堆
    first_block = (block_header_t*)kernel_heap.start;
    first_block->size = kernel_heap.size;
    first_block->is_free = 1;
    first_block->next = NULL;
    first_block->prev = NULL;
    
    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("OK\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr("  Heap: ");
    screen_puthex(kernel_heap.start);
    screen_putstr(" - ");
    screen_puthex(kernel_heap.start + kernel_heap.size);
    screen_putstr(" (");
    screen_putdec(kernel_heap.size / 1024 / 1024);
    screen_putstr(" MB)\n");
}

// 分配内存
void* kmalloc(uint32_t size) {
    // 对齐到8字节
    size = (size + 7) & ~7;
    size += sizeof(block_header_t);
    
    // 查找合适的空闲块
    block_header_t* current = (block_header_t*)kernel_heap.start;
    
    while (current) {
        if (current->is_free && current->size >= size) {
            // 找到合适的块
            
            // 如果块足够大，可以分割
            if (current->size >= size + sizeof(block_header_t) + 8) {
                // 创建新块
                block_header_t* new_block = (block_header_t*)((uint32_t)current + size);
                new_block->size = current->size - size;
                new_block->is_free = 1;
                new_block->next = current->next;
                new_block->prev = current;
                
                if (current->next) {
                    current->next->prev = new_block;
                }
                
                current->size = size;
                current->next = new_block;
            }
            
            current->is_free = 0;
            kernel_heap.used += current->size;
            
            // 返回数据区域
            return (void*)((uint32_t)current + sizeof(block_header_t));
        }
        
        current = current->next;
    }
    
    // 没有找到合适的块
    screen_setcolor(COLOR_YELLOW, COLOR_BLACK);
    screen_putstr("Heap fragmentation! Attempting to expand heap...\n");
    
    // TODO: 扩展堆
    return NULL;
}

// 重新分配内存
void* krealloc(void* ptr, uint32_t size) {
    if (!ptr) {
        return kmalloc(size);
    }
    
    if (size == 0) {
        kfree(ptr);
        return NULL;
    }
    
    // 获取块头部
    block_header_t* block = (block_header_t*)((uint32_t)ptr - sizeof(block_header_t));
    uint32_t old_size = block->size - sizeof(block_header_t);
    
    // 如果新大小小于等于旧大小，直接返回
    if (size <= old_size) {
        return ptr;
    }
    
    // 分配新内存
    void* new_ptr = kmalloc(size);
    if (!new_ptr) {
        return NULL;
    }
    
    // 复制数据
    memcpy(new_ptr, ptr, old_size);
    
    // 释放旧内存
    kfree(ptr);
    
    return new_ptr;
}

// 分配并清零内存
void* kcalloc(uint32_t num, uint32_t size) {
    uint32_t total = num * size;
    void* ptr = kmalloc(total);
    
    if (ptr) {
        memset(ptr, 0, total);
    }
    
    return ptr;
}

// 释放内存
void kfree(void* ptr) {
    if (!ptr) {
        return;
    }
    
    // 获取块头部
    block_header_t* block = (block_header_t*)((uint32_t)ptr - sizeof(block_header_t));
    
    if (block->is_free) {
        screen_setcolor(COLOR_YELLOW, COLOR_BLACK);
        screen_putstr("Warning: Double free detected!\n");
        return;
    }
    
    // 标记为空闲
    block->is_free = 1;
    kernel_heap.used -= block->size;
    
    // 合并相邻的空闲块
    // 向后合并
    if (block->next && block->next->is_free) {
        block->size += block->next->size;
        block->next = block->next->next;
        
        if (block->next) {
            block->next->prev = block;
        }
    }
    
    // 向前合并
    if (block->prev && block->prev->is_free) {
        block->prev->size += block->size;
        block->prev->next = block->next;
        
        if (block->next) {
            block->next->prev = block->prev;
        }
    }
}

// 显示堆状态
void show_heap_status(void) {
    screen_setcolor(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_putstr("\nHeap Status:\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    
    screen_putstr("  Start: ");
    screen_puthex(kernel_heap.start);
    screen_putstr("\n");
    screen_putstr("  Size:  ");
    screen_putdec(kernel_heap.size / 1024);
    screen_putstr(" KB\n");
    screen_putstr("  Used:  ");
    screen_putdec(kernel_heap.used / 1024);
    screen_putstr(" KB (");
    screen_putdec(kernel_heap.used * 100 / kernel_heap.size);
    screen_putstr("%)\n");
    screen_putstr("  Free:  ");
    screen_putdec((kernel_heap.size - kernel_heap.used) / 1024);
    screen_putstr(" KB\n");
    
    // 显示块信息
    screen_putstr("\nMemory blocks:\n");
    block_header_t* block = (block_header_t*)kernel_heap.start;
    uint32_t block_count = 0;
    uint32_t free_blocks = 0;
    
    while (block) {
        screen_putstr("  Block ");
        screen_putdec(block_count);
        screen_putstr(": ");
        screen_puthex((uint32_t)block);
        screen_putstr(" - ");
        screen_puthex((uint32_t)block + block->size);
        screen_putstr(" (");
        screen_putdec(block->size);
        screen_putstr(" bytes) ");
        screen_putstr(block->is_free ? "[FREE]" : "[USED]");
        screen_putstr("\n");
        
        block_count++;
        if (block->is_free) free_blocks++;
        block = block->next;
    }
    
    screen_putstr("  Total blocks: ");
    screen_putdec(block_count);
    screen_putstr(", Free blocks: ");
    screen_putdec(free_blocks);
    screen_putstr("\n");
}