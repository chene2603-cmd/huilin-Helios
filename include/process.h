#ifndef PROCESS_H
#define PROCESS_H

#include "system.h"
#include "paging.h"

#define MAX_PROCESS_NAME 32
#define MAX_FD          16

typedef enum {
    PROCESS_NEW = 0,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

typedef struct process_control_block {
    // ---- 上下文切换专用 (偏移 0/4/8, switch.asm 依赖) ----
    uint32_t kernel_esp;         // 偏移 0 : 保存内核栈指针
    uint32_t eflags;             // 偏移 4 : 保存标志寄存器
    uint32_t page_dir_phys;      // 偏移 8 : 页目录物理地址

    // ---- 原 PCB 字段 ----
    uint32_t pid;
    char     name[MAX_PROCESS_NAME];
    process_state_t state;

    // 完整寄存器上下文（用户态 fork 时拷贝）
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip;
    uint32_t cs, ds, es, fs, gs, ss;

    // 内存
    page_directory_t* page_dir;
    uint32_t brk;

    // 文件描述符
    int32_t fd[MAX_FD];

    // 进程关系
    struct process_control_block* parent;
    struct process_control_block* children;
    struct process_control_block* sibling;
    struct process_control_block* next;   // 就绪队列链表

    // 调度
    uint32_t time_slice;
    uint32_t sleep_until;

    // 退出信息
    int32_t exit_code;
} pcb_t;

// 进程管理函数声明
void init_process_manager(void);
pcb_t* get_current_process(void);
int32_t get_pid(void);
pcb_t* fork_process(void);
void exit_process(int exit_code);
void wait_process(int pid);
void sleep_process(uint32_t ms);
int32_t kill_process(int pid, int sig);
void yield(void);
pcb_t* scheduler_get_next(void);
void add_to_ready_queue(pcb_t* proc);
pcb_t* create_process(const char* name, void* entry, uint32_t stack_top);
void init_scheduler(void);

// switch.asm 中的上下文切换
extern void switch_process(pcb_t* next);

#endif