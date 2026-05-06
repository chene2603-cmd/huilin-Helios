#include "system.h"
#include "process.h"
#include "screen.h"
#include "string.h"

// 引用 paging.c 中的内核页目录（必须存在）
extern page_directory_t kernel_page_dir;

#define MAX_PROCESSES 64

static pcb_t process_table[MAX_PROCESSES];
static pcb_t *current_process = NULL;
static uint32_t next_pid = 1;
static pcb_t *ready_queue = NULL;

// ---------- 内部辅助 ----------
static void enqueue_process(pcb_t *proc) {
    proc->next = NULL;
    if (!ready_queue) {
        ready_queue = proc;
    } else {
        pcb_t *p = ready_queue;
        while (p->next) p = p->next;
        p->next = proc;
    }
}

static pcb_t *dequeue_process(void) {
    if (!ready_queue) return NULL;
    pcb_t *p = ready_queue;
    ready_queue = p->next;
    p->next = NULL;
    return p;
}

// ---------- 调度器 ----------
pcb_t *scheduler_get_next(void) {
    if (current_process && current_process->state == PROCESS_RUNNING) {
        current_process->state = PROCESS_READY;
        enqueue_process(current_process);
    }
    pcb_t *next = dequeue_process();
    // 如果没有就绪进程，去 idle
    if (!next) {
        // idle 进程始终在 process_table[0]
        next = &process_table[0];
        next->state = PROCESS_RUNNING;
    } else {
        next->state = PROCESS_RUNNING;
    }
    current_process = next;
    return next;
}

// ---------- 获取当前进程 ----------
pcb_t *get_current_process(void) {
    return current_process;
}

int32_t get_pid(void) {
    if (!current_process) return 0;
    return current_process->pid;
}

// ---------- 初始化进程子系统 ----------
void init_process_manager(void) {
    screen_putstr("Initializing Process Manager... ");
    memset(process_table, 0, sizeof(process_table));
    ready_queue = NULL;

    // 创建 idle 进程 (PID 0)
    pcb_t *idle = &process_table[0];
    idle->pid = 0;
    strcpy(idle->name, "idle");
    idle->state = PROCESS_RUNNING;
    idle->page_dir = &kernel_page_dir;
    idle->brk = 0x100000;
    idle->time_slice = 10;
    for (int i = 0; i < 16; i++) idle->fd[i] = -1;
    current_process = idle;
    next_pid = 1;

    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("OK\n");
}

// ---------- 创建空进程结构 ----------
static pcb_t *create_process_empty(void) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].state == PROCESS_TERMINATED || process_table[i].pid == 0) {
            pcb_t *p = &process_table[i];
            memset(p, 0, sizeof(pcb_t));
            p->pid = next_pid++;
            p->state = PROCESS_NEW;
            p->time_slice = 10;
            for (int j = 0; j < 16; j++) p->fd[j] = -1;
            return p;
        }
    }
    return NULL;
}

// ---------- fork ----------
pcb_t *fork_process(void) {
    if (!current_process) return NULL;
    pcb_t *child = create_process_empty();
    if (!child) return NULL;

    // 拷贝父进程信息
    strcpy(child->name, current_process->name);
    child->parent = current_process;
    child->page_dir = current_process->page_dir;   // 共享页目录（简化）
    child->brk = current_process->brk;

    // 复制文件描述符表
    for (int i = 0; i < 16; i++)
        child->fd[i] = current_process->fd[i];

    // 复制寄存器上下文（这些字段来自 process.h）
    child->eip = current_process->eip;
    child->esp = current_process->esp;
    child->ebp = current_process->ebp;
    child->eax = 0;           // 子进程返回 0
    child->ecx = current_process->ecx;
    child->edx = current_process->edx;
    child->ebx = current_process->ebx;
    child->esi = current_process->esi;
    child->edi = current_process->edi;
    child->eflags = current_process->eflags;
    child->cs = current_process->cs;
    child->ds = current_process->ds;
    child->es = current_process->es;
    child->fs = current_process->fs;
    child->gs = current_process->gs;
    child->ss = current_process->ss;

    child->state = PROCESS_READY;
    enqueue_process(child);
    return child;
}

// ---------- exit ----------
void exit_process(int exit_code) {
    if (!current_process) return;
    current_process->exit_code = exit_code;
    current_process->state = PROCESS_TERMINATED;
    scheduler_get_next();   // 强制切换，永不返回
}

// ---------- wait ----------
void wait_process(int pid) {
    // 简化：直接将当前进程置为阻塞，等待被唤醒
    current_process->state = PROCESS_BLOCKED;
    scheduler_get_next();
}

// ---------- sleep ----------
void sleep_process(uint32_t ms) {
    // 如果有全局滴答计数器可用则设置唤醒时间
    // 否则直接阻塞（最简单的实现）
    (void)ms;
    current_process->state = PROCESS_BLOCKED;
    scheduler_get_next();
}

// ---------- kill ----------
int32_t kill_process(int pid, int sig) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid &&
            process_table[i].state != PROCESS_TERMINATED) {
            process_table[i].state = PROCESS_TERMINATED;
            return 0;
        }
    }
    return -1;
}

// ---------- yield ----------
void yield(void) {
    asm volatile("int $0x20");   // 假设 0x20 是调度中断
}

// ---------- 其他需要的接口 ----------
void add_to_ready_queue(pcb_t *proc) {
    if (proc) {
        proc->state = PROCESS_READY;
        enqueue_process(proc);
    }
}

// 创建一个简单的用户进程（给 kernel.c 使用）
pcb_t *create_process(const char *name, void *entry, uint32_t stack_top) {
    pcb_t *p = create_process_empty();
    if (!p) return NULL;

    strcpy(p->name, name);
    p->eip = (uint32_t)entry;
    p->esp = stack_top;
    p->ebp = 0;
    p->page_dir = current_process ? current_process->page_dir : &kernel_page_dir;
    p->brk = 0x100000;
    p->state = PROCESS_READY;
    enqueue_process(p);
    return p;
}

// 初始化调度器（占位）
void init_scheduler(void) {
    // 已在 init_process_manager 中完成
}