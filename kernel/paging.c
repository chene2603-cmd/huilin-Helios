#include "paging.h"
#include "pmm.h"
#include "screen.h"
#include "string.h"

// 内核页目录（全局，其他模块通过 extern 引用）
page_directory_t kernel_page_dir;

// 当前使用的页目录（可切换，初始指向内核页目录）
page_directory_t *current_page_dir = &kernel_page_dir;

// 分配一个页表并清零
static page_table_t *create_page_table(void) {
    page_table_t *table = (page_table_t *)pmm_alloc_page();
    if (!table) return NULL;
    memset(table, 0, sizeof(page_table_t));
    return table;
}

// ---- 初始化分页 ----
void init_paging(void) {
    screen_putstr("Initializing Paging... ");

    // 清零内核页目录
    memset(&kernel_page_dir, 0, sizeof(page_directory_t));

    // 恒等映射低 4 MB（内核空间）
    page_table_t *identity_table = create_page_table();
    if (!identity_table) {
        screen_setcolor(COLOR_RED, COLOR_BLACK);
        screen_putstr("FAILED\n");
        return;
    }

    uint32_t phys = 0;
    for (int i = 0; i < 1024; i++) {
        identity_table->pages[i].present = 1;
        identity_table->pages[i].rw = 1;
        identity_table->pages[i].frame = phys >> 12;
        phys += 0x1000;
    }

    // 设置页目录第一项
    kernel_page_dir.entries[0].present = 1;
    kernel_page_dir.entries[0].rw = 1;
    kernel_page_dir.entries[0].frame = (uint32_t)identity_table >> 12;
    kernel_page_dir.tables[0] = identity_table;

    // 加载页目录到 CR3
    asm volatile("mov %0, %%cr3" :: "r"(&kernel_page_dir));

    // 启用分页
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;   // PG 位
    asm volatile("mov %0, %%cr0" :: "r"(cr0));

    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("OK\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
}

// ---- 映射虚拟页到物理页 ----
void map_page(void *vaddr, void *paddr, uint32_t flags) {
    uint32_t v = (uint32_t)vaddr;
    uint32_t pd_index = v >> 22;
    uint32_t pt_index = (v >> 12) & 0x3FF;

    page_directory_t *dir = current_page_dir;
    if (!dir) dir = &kernel_page_dir;

    // 如果页目录项不存在，分配新页表
    if (!dir->tables[pd_index]) {
        page_table_t *table = create_page_table();
        if (!table) return;
        dir->tables[pd_index] = table;
        dir->entries[pd_index].present = 1;
        dir->entries[pd_index].rw = (flags & PAGE_RW) ? 1 : 0;
        dir->entries[pd_index].user = (flags & PAGE_USER) ? 1 : 0;
        dir->entries[pd_index].frame = (uint32_t)table >> 12;
    }

    page_table_t *table = dir->tables[pd_index];
    table->pages[pt_index].present = 1;
    table->pages[pt_index].rw = (flags & PAGE_RW) ? 1 : 0;
    table->pages[pt_index].user = (flags & PAGE_USER) ? 1 : 0;
    table->pages[pt_index].frame = (uint32_t)paddr >> 12;
}

// ---- 取消虚拟页映射 ----
void unmap_page(void *vaddr) {
    uint32_t v = (uint32_t)vaddr;
    uint32_t pd_index = v >> 22;
    uint32_t pt_index = (v >> 12) & 0x3FF;

    page_directory_t *dir = current_page_dir;
    if (!dir) dir = &kernel_page_dir;

    if (dir->tables[pd_index]) {
        dir->tables[pd_index]->pages[pt_index].present = 0;
        dir->tables[pd_index]->pages[pt_index].frame = 0;
        // 刷新 TLB
        asm volatile("invlpg (%0)" :: "r"(vaddr) : "memory");
    }
}

// ---- 切换页目录（用于进程切换）----
void switch_page_directory(page_directory_t *dir) {
    if (!dir) return;
    current_page_dir = dir;
    asm volatile("mov %0, %%cr3" :: "r"(dir));
}