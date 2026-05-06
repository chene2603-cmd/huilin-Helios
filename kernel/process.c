#include "system.h"
#include "process.h"
#include "pmm.h"
#include "vmm.h"
#include "kheap.h"
#include "screen.h"
#include "string.h"

#define MAX_PROCESSES 64

static pcb_t process_table[MAX_PROCESSES];
static pcb_t *current_process = NULL;
static uint32_t next_pid = 1;

// 进程调度队列头（简单链表）
static pcb_t *ready_queue = NULL;

// 内部函数：将进程加入就绪队列尾部
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

// 内部函数：从就绪队列头部取出进程
static pcb_t *dequeue_process(void) {
    if (!ready_queue) return NULL;
    pcb_t *p = ready_queue;
    ready_queue = p->next;
    p->next = NULL;
    return p;
}

// 调度器：选择下一个进程
pcb_t *scheduler_get_next(void) {
    if (current_process && current_process->state == PROCESS_RUNNING) {
        current_process->state = PROCESS_READY;
        enqueue_process(current_process);
    }
    pcb_t *next = dequeue_process();
    if (!next) {
        // 就绪队列空，让 idle 进程运行（如果实现）或 panic
        screen_putstr("No ready process!\n");
        for(;;);
    }
    next->state = PROCESS_RUNNING;
    current_process = next;
    return next;
}

// 获取当前进程
pcb_t *get_current_process(void) {
    return current_process;
}

// 获取 PID
int32_t get_pid(void) {
    if (!current_process) return 0;
    return current_process->pid;
}

// 初始化进程子系统
void init_process(void) {
    screen_putstr("Initializing Process Subsystem... ");
    memset(process_table, 0, sizeof(process_table));
    ready_queue = NULL;

    // 创建内核 idle 进程（PID 0）
    pcb_t *idle = &process_table[0];
    idle->pid = 0;
    strcpy(idle->name, "idle");
    idle->state = PROCESS_RUNNING;
    idle->page_dir = vmm_get_kernel_directory();  // 内核页目录
    idle->brk = 0x100000;  // 示例
    idle->time_slice = 10;
    for (int i = 0; i < 16; i++) idle->fd[i] = -1;
    current_process = idle;
    next_pid = 1;

    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("OK\n");
}

// 创建一个新进程（不复制内存，仅分配 PCB）
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

// 复制页目录和用户空间内存（简化：同一页目录共享，仅标记写时复制 --- 这里简化为完全复制）
static page_directory_t *clone_user_space(page_directory_t *src) {
    // 简化实现：直接使用同一个页目录（真实系统应执行 COW）
    // 为简单起见，我们直接返回内核目录，并映射相同的物理页
    // 这仅用于演示，真实 fork 需要复制页表和物理内存
    return src;
}

// fork 进程
pcb_t *fork_process(void) {
    if (!current_process) return NULL;

    pcb_t *child = create_process_empty();
    if (!child) return NULL;

    // 复制父进程信息
    strcpy(child->name, current_process->name);
    child->parent = current_process;
    child->page_dir = clone_user_space(current_process->page_dir);
    child->brk = current_process->brk;

    // 复制文件描述符表
    for (int i = 0; i < 16; i++) child->fd[i] = current_process->fd[i];

    // 复制寄存器上下文（栈，指令指针等）
    child->eip = current_process->eip;
    child->esp = current_process->esp;
    child->ebp = current_process->ebp;
    child->eax = 0;  // 子进程返回 0
    child->ecx = current_process->ecx;
    child->edx = current_process->edx;
    child->ebx = current_process->ebx;
    child->esi = current_process->esi;
    child->edi = current_process->edi;

    child->cs = current_process->cs;
    child->ds = current_process->ds;
    child->es = current_process->es;
    child->fs = current_process->fs;
    child->gs = current_process->gs;
    child->ss = current_process->ss;

    child->eflags = current_process->eflags;
    child->state = PROCESS_READY;
    enqueue_process(child);

    // 父进程返回子进程 PID（在 sys_fork 中处理）
    return child;
}

// 退出进程
void exit_process(int exit_code) {
    if (!current_process) return;

    current_process->exit_code = exit_code;
    current_process->state = PROCESS_TERMINATED;

    // 唤醒可能等待的父进程
    // (此处简化，真正的 waitpid 会检查)

    // 强制调度
    scheduler_get_next();
}

// 等待子进程
void wait_process(int pid) {
    (void)pid;
    // 简化实现：将当前进程阻塞，直到子进程退出（由调度器处理）
    current_process->state = PROCESS_BLOCKED;
    // 可以添加等待队列，这里直接让出CPU
    scheduler_get_next();
}

// 睡眠（毫秒）
void sleep_process(uint32_t ms) {
    // 简化：将进程标记为阻塞，由定时器唤醒
    current_process->sleep_until = get_time_ms() + ms;
    current_process->state = PROCESS_BLOCKED;
    scheduler_get_next();
}

// 杀死进程
int32_t kill_process(int pid, int sig) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && process_table[i].state != PROCESS_TERMINATED) {
            process_table[i].state = PROCESS_TERMINATED;
            return 0;
        }
    }
    return -1;
}

// 主动让出 CPU
void yield(void) {
    asm volatile("int $0x20");  // 假设 0x20 是调度中断
}