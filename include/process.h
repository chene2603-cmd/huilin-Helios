#ifndef PROCESS_H
#define PROCESS_H

#include "system.h"
#include "paging.h"

#define MAX_PROCESS_NAME 32
#define MAX_FD 16

typedef enum {
    PROCESS_NEW = 0,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_BLOCKED,
    PROCESS_TERMINATED
} process_state_t;

typedef struct process_control_block {
    uint32_t pid;
    char name[MAX_PROCESS_NAME];
    process_state_t state;

    // 完整寄存器上下文（用于任务切换）
    uint32_t eax, ebx, ecx, edx;
    uint32_t esi, edi, ebp, esp;
    uint32_t eip, eflags;
    uint32_t cs, ds, es, fs, gs, ss;

    // 内存
    page_directory_t* page_dir;
    uint32_t brk;                  // 堆顶

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

// 进程管理函数（由 kernel/process.c 实现）
void init_process(void);
pcb_t* get_current_process(void);
int32_t get_pid(void);
pcb_t* fork_process(void);
void exit_process(int exit_code);
void wait_process(int pid);
void sleep_process(uint32_t ms);
int32_t kill_process(int pid, int sig);
void yield(void);
pcb_t* scheduler_get_next(void);

#endif