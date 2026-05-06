#include "system.h"
#include "screen.h"
#include "paging.h"
#include "pmm.h"
#include "isr.h"

// 当前页面目录
static page_directory_t* current_directory = NULL;
static page_directory_t kernel_directory;

// 初始化分页机制
void init_paging(void) {
    screen_putstr("Initializing Paging... ");
    
    // 1. 为内核创建页面目录
    current_directory = &kernel_directory;
    memset(current_directory, 0, sizeof(page_directory_t));
    
    // 2. 标识前4MB为内核空间（恒等映射）
    for (uint32_t i = 0; i < 1024; i++) {
        page_dir_entry_t* entry = &current_directory->entries[i];
        entry->present = 1;
        entry->rw = 1;
        entry->user = 0;  // 内核模式
        entry->page_size = 0;  // 4KB页面
        entry->frame = i;  // 恒等映射：虚拟地址=物理地址
    }
    
    // 3. 设置CR3寄存器
    switch_page_directory(current_directory);
    
    // 4. 启用分页
    enable_paging();
    
    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("OK\n");
}

// 切换页面目录
void switch_page_directory(page_directory_t* dir) {
    if (!dir) return;
    
    current_directory = dir;
    asm volatile("mov %0, %%cr3" : : "r"(dir));
}

// 获取当前页面目录
page_directory_t* get_current_page_dir(void) {
    return current_directory;
}

// 获取内核页面目录
page_directory_t* get_kernel_page_dir(void) {
    return &kernel_directory;
}

// 映射虚拟页面到物理页面
void map_page(page_directory_t* dir, void* virt, void* phys, uint32_t flags) {
    uint32_t virt_addr = (uint32_t)virt;
    uint32_t phys_addr = (uint32_t)phys;
    
    // 计算目录索引和表索引
    uint32_t dir_index = virt_addr >> 22;
    uint32_t table_index = (virt_addr >> 12) & 0x3FF;
    
    // 获取目录项
    page_dir_entry_t* dir_entry = &dir->entries[dir_index];
    
    // 如果页面表不存在，创建它
    if (!dir_entry->present) {
        // 分配物理页框给页面表
        page_table_entry_t* table = (page_table_entry_t*)alloc_frame();
        if (!table) {
            screen_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
            screen_putstr("Failed to allocate page table!\n");
            return;
        }
        
        // 清空页面表
        memset(table, 0, PAGE_TABLE_SIZE);
        
        // 设置目录项
        dir_entry->present = 1;
        dir_entry->rw = 1;
        dir_entry->user = (flags & PAGE_USER) ? 1 : 0;
        dir_entry->frame = ((uint32_t)table) >> 12;
    }
    
    // 获取页面表
    page_table_entry_t* table = (page_table_entry_t*)(dir_entry->frame << 12);
    page_table_entry_t* table_entry = &table[table_index];
    
    // 设置页面表项
    table_entry->present = 1;
    table_entry->rw = (flags & PAGE_WRITE) ? 1 : 0;
    table_entry->user = (flags & PAGE_USER) ? 1 : 0;
    table_entry->frame = phys_addr >> 12;
    
    // 刷新TLB
    asm volatile("invlpg (%0)" : : "r"(virt_addr) : "memory");
}

// 取消页面映射
void unmap_page(page_directory_t* dir, void* virt) {
    uint32_t virt_addr = (uint32_t)virt;
    uint32_t dir_index = virt_addr >> 22;
    uint32_t table_index = (virt_addr >> 12) & 0x3FF;
    
    page_dir_entry_t* dir_entry = &dir->entries[dir_index];
    if (!dir_entry->present) {
        return;
    }
    
    page_table_entry_t* table = (page_table_entry_t*)(dir_entry->frame << 12);
    page_table_entry_t* table_entry = &table[table_index];
    
    table_entry->present = 0;
    
    // 刷新TLB
    asm volatile("invlpg (%0)" : : "r"(virt_addr) : "memory");
}

// 分配一个虚拟页面
void* alloc_page(uint32_t flags) {
    // 分配物理页框
    void* phys = alloc_frame();
    if (!phys) {
        return NULL;
    }
    
    // 查找空闲的虚拟地址（简化：从3GB开始）
    static uint32_t next_virt = 0xC0000000;  // 3GB
    void* virt = (void*)next_virt;
    next_virt += PAGE_SIZE;
    
    // 建立映射
    map_page(current_directory, virt, phys, flags);
    
    return virt;
}

// 释放虚拟页面
void free_page(void* virt) {
    // 获取物理地址
    void* phys = get_physical_address(current_directory, virt);
    if (phys) {
        free_frame(phys);
    }
    
    // 取消映射
    unmap_page(current_directory, virt);
}

// 获取虚拟地址对应的物理地址
void* get_physical_address(page_directory_t* dir, void* virt) {
    uint32_t virt_addr = (uint32_t)virt;
    uint32_t dir_index = virt_addr >> 22;
    uint32_t table_index = (virt_addr >> 12) & 0x3FF;
    
    page_dir_entry_t* dir_entry = &dir->entries[dir_index];
    if (!dir_entry->present) {
        return NULL;
    }
    
    page_table_entry_t* table = (page_table_entry_t*)(dir_entry->frame << 12);
    page_table_entry_t* table_entry = &table[table_index];
    
    if (!table_entry->present) {
        return NULL;
    }
    
    uint32_t phys_addr = (table_entry->frame << 12) | (virt_addr & 0xFFF);
    return (void*)phys_addr;
}

// 启用分页
void enable_paging(void) {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 |= 0x80000000;  // 设置PG位
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

// 禁用分页
void disable_paging(void) {
    uint32_t cr0;
    asm volatile("mov %%cr0, %0" : "=r"(cr0));
    cr0 &= ~0x80000000;  // 清除PG位
    asm volatile("mov %0, %%cr0" : : "r"(cr0));
}

// 页面错误处理程序
void page_fault_handler(registers_t* regs) {
    uint32_t faulting_address;
    asm volatile("mov %%cr2, %0" : "=r"(faulting_address));
    
    screen_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
    screen_putstr("\nPage Fault!");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr(" at ");
    screen_puthex(faulting_address);
    screen_putstr("\n");
    
    // 解析错误码
    uint32_t present = regs->err_code & 0x1;
    uint32_t rw = regs->err_code & 0x2;
    uint32_t user = regs->err_code & 0x4;
    uint32_t reserved = regs->err_code & 0x8;
    uint32_t id = regs->err_code & 0x10;
    
    screen_putstr("Error code: ");
    if (!present) screen_putstr("not present ");
    if (rw) screen_putstr("write ");
    if (user) screen_putstr("user mode ");
    if (reserved) screen_putstr("reserved ");
    if (id) screen_putstr("instruction fetch ");
    screen_putstr("\n");
    
    screen_putstr("EIP: ");
    screen_puthex(regs->eip);
    screen_putstr("\n");
    
    // 如果是在内核模式下的页面错误，死机
    if (!(regs->cs & 0x03)) {
        screen_putstr("Kernel page fault! System halted.\n");
        while(1) asm volatile("hlt");
    }
    
    // 对于用户模式，可以杀死进程（TODO）
    screen_putstr("Terminating process...\n");
    
    // 临时修复：分配页面并继续
    void* page = alloc_page(PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    if (page) {
        screen_putstr("Allocated page at ");
        screen_puthex((uint32_t)page);
        screen_putstr("\n");
    } else {
        screen_putstr("Failed to allocate page\n");
        while(1) asm volatile("hlt");
    }
}