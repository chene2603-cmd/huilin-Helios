#include "vmm.h"
#include "pmm.h"
#include "paging.h"
#include "string.h"
#include "screen.h"

// 假设你的 paging.c 中定义了内核页目录全局变量 kernel_page_dir
extern page_directory_t kernel_page_dir;

// 当前使用的页目录（默认内核页目录）
static page_directory_t *current_pd = &kernel_page_dir;

// 获取内核页目录（供 syscall.c 使用）
page_directory_t *vmm_get_kernel_directory(void) {
    return &kernel_page_dir;
}

// 初始化 VMM（分页已在 paging_init 中启用，这里可能无需操作）
void init_vmm(void) {
    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("VMM initialized\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
}

// 映射虚拟页到物理页
void vmm_map_page(void *vaddr, void *paddr, uint32_t flags) {
    // 直接调用你 paging.c 中的 map_page 函数
    extern void map_page(void *vaddr, void *paddr, uint32_t flags);
    map_page(vaddr, paddr, flags);
}

// 解除虚拟页映射
void vmm_unmap_page(void *vaddr) {
    extern void unmap_page(void *vaddr);
    unmap_page(vaddr);
}