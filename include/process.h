#ifndef PROCESS_H
#define PROCESS_H

#include "system.h"
#include "paging.h"

// 进程状态
typedef enum {
    PROCESS_NEW,        // 新建
    PROCESS_READY,      // 就绪
    PROCESS_RUNNING,    // 运行
    PROCESS_BLOCKED,    // 阻塞
    PROCESS_ZOMBIE,     // 僵尸
    PROCESS_TERMINATED  // 终止
} process_state_t;

// 进程控制块
typedef struct process_control_block {
    uint32_t pid;                     // 进程ID
    char name[32];                    // 进程名
    process_state_t state;            // 进程状态
    
    // 寄存器上下文
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi;
    uint32_t esp, ebp;
    uint32_t eip;
    uint32_t eflags;
    
    // 段寄存器
    uint32_t cs, ds, es, fs, gs, ss;
    
    // 内存管理
    page_directory_t* page_dir;       // 页目录
    uint32_t brk;                     // 堆顶指针
    
    // 文件描述符
    int32_t fd[16];                   // 文件描述符表
    
    // 进程关系
    struct process_control_block* parent;  // 父进程
    struct process_control_block* children;// 子进程列表
    struct process_control_block* sibling; // 兄弟进程
    
    // 调度信息
    uint32_t priority;                // 优先级
    uint32_t time_slice;              // 时间片
    uint32_t runtime;                 // 运行时间
    uint32_t wakeup_time;             // 唤醒时间
    
    // 退出状态
    int32_t exit_code;                // 退出码
    
    // 链表指针
    struct process_control_block* next;
} pcb_t;

// 进程管理函数
void init_process_manager(void);
pcb_t* create_process(const char* name, void* entry_point);
pcb_t* fork_process(void);
void exit_process(int exit_code);
void wait_process(uint32_t pid);
void kill_process(uint32_t pid, int signal);
pcb_t* get_current_process(void);
uint32_t get_pid(void);
void switch_process(pcb_t* new_process);
void sleep_process(uint32_t milliseconds);
void wakeup_process(pcb_t* process);

// 进程调度
void schedule(void);
void yield(void);

#endif