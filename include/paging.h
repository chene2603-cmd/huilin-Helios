#ifndef PAGING_H
#define PAGING_H

#include "system.h"

// 分页相关常量
#define PAGE_SIZE 4096
#define PAGE_ENTRIES 1024
#define PAGE_DIR_SIZE (PAGE_ENTRIES * sizeof(page_dir_entry_t))
#define PAGE_TABLE_SIZE (PAGE_ENTRIES * sizeof(page_table_entry_t))

// 页面属性标志
#define PAGE_PRESENT   0x01
#define PAGE_WRITE     0x02
#define PAGE_USER      0x04
#define PAGE_WRITETHROUGH 0x08
#define PAGE_CACHEDISABLE 0x10
#define PAGE_ACCESSED  0x20
#define PAGE_DIRTY     0x40
#define PAGE_SIZE_4MB  0x80
#define PAGE_GLOBAL    0x100

// 页面目录项结构
typedef struct {
    uint32_t present : 1;     // 页面是否在内存中
    uint32_t rw : 1;          // 读写权限
    uint32_t user : 1;        // 用户/内核模式
    uint32_t write_through : 1; // 直写缓存
    uint32_t cache_disable : 1; // 禁用缓存
    uint32_t accessed : 1;     // 是否被访问
    uint32_t zero : 1;         // 保留
    uint32_t page_size : 1;    // 页面大小(0=4KB,1=4MB)
    uint32_t ignored : 1;      // 忽略
    uint32_t available : 3;    // 可用位
    uint32_t frame : 20;       // 物理页框地址高20位
} __attribute__((packed)) page_dir_entry_t;

// 页面表项结构
typedef struct {
    uint32_t present : 1;
    uint32_t rw : 1;
    uint32_t user : 1;
    uint32_t write_through : 1;
    uint32_t cache_disable : 1;
    uint32_t accessed : 1;
    uint32_t dirty : 1;       // 是否被写入
    uint32_t zero : 2;        // 保留
    uint32_t available : 3;
    uint32_t frame : 20;      // 物理页框地址高20位
} __attribute__((packed)) page_table_entry_t;

// 页面目录
typedef struct {
    page_dir_entry_t entries[PAGE_ENTRIES];
} page_directory_t;

// 函数声明
void init_paging(void);
void switch_page_directory(page_directory_t* dir);
page_directory_t* get_kernel_page_dir(void);
void* alloc_page(uint32_t flags);
void free_page(void* addr);
void map_page(page_directory_t* dir, void* virt, void* phys, uint32_t flags);
void unmap_page(page_directory_t* dir, void* virt);
void* get_physical_address(page_directory_t* dir, void* virt);
void enable_paging(void);
void disable_paging(void);
void page_fault_handler(registers_t* regs);

#endif