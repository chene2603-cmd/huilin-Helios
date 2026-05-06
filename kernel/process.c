#include "system.h"
#include "screen.h"
#include "process.h"
#include "scheduler.h"
#include "kheap.h"
#include "paging.h"
#include "string.h"

// 全局变量
static pcb_t* current_process = NULL;
static pcb_t* process_list = NULL;
static uint32_t next_pid = 1;
static pcb_t* init_process = NULL;

// 初始化进程管理器
void init_process_manager(void) {
    screen_putstr("Initializing Process Manager... ");
    
    process_list = NULL;
    next_pid = 1;
    
    // 创建初始进程（内核空闲进程）
    init_process = create_process("idle", NULL);
    if (!init_process) {
        screen_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
        screen_putstr("Failed to create idle process!\n");
        return;
    }
    
    init_process->state = PROCESS_RUNNING;
    current_process = init_process;
    
    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("OK\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr("  Idle process PID: ");
    screen_putdec(init_process->pid);
    screen_putstr("\n");
}

// 创建新进程
pcb_t* create_process(const char* name, void* entry_point) {
    // 分配PCB
    pcb_t* proc = (pcb_t*)kmalloc(sizeof(pcb_t));
    if (!proc) {
        screen_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
        screen_putstr("Failed to allocate PCB!\n");
        return NULL;
    }
    
    // 初始化PCB
    memset(proc, 0, sizeof(pcb_t));
    
    proc->pid = next_pid++;
    strncpy(proc->name, name, sizeof(proc->name) - 1);
    proc->name[sizeof(proc->name) - 1] = '\0';
    proc->state = PROCESS_NEW;
    
    // 初始化页目录
    proc->page_dir = (page_directory_t*)kmalloc(sizeof(page_directory_t));
    if (!proc->page_dir) {
        kfree(proc);
        return NULL;
    }
    
    // 复制内核页目录
    memcpy(proc->page_dir, get_kernel_page_dir(), sizeof(page_directory_t));
    
    // 设置用户空间
    // 用户空间：0x00000000 - 0xBFFFFFFF (3GB)
    // 内核空间：0xC0000000 - 0xFFFFFFFF (1GB)
    
    // 分配用户栈
    void* user_stack = alloc_page(PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    if (!user_stack) {
        kfree(proc->page_dir);
        kfree(proc);
        return NULL;
    }
    
    // 分配用户代码页
    void* user_code = alloc_page(PAGE_PRESENT | PAGE_WRITE | PAGE_USER);
    if (!user_code) {
        free_page(user_stack);
        kfree(proc->page_dir);
        kfree(proc);
        return NULL;
    }
    
    // 设置寄存器上下文
    if (entry_point) {
        // 用户进程
        proc->eip = (uint32_t)entry_point;
        proc->esp = (uint32_t)user_stack + PAGE_SIZE - 16;  // 栈顶
        proc->ebp = proc->esp;
        
        // 用户模式段选择子
        proc->cs = 0x1B;  // 用户代码段
        proc->ds = 0x23;  // 用户数据段
        proc->es = 0x23;
        proc->fs = 0x23;
        proc->gs = 0x23;
        proc->ss = 0x23;
        
        // 启用中断
        proc->eflags = 0x200;  // 启用中断
    } else {
        // 内核进程
        proc->eip = 0;
        proc->esp = 0;
        proc->ebp = 0;
        
        // 内核段选择子
        proc->cs = 0x08;
        proc->ds = 0x10;
        proc->es = 0x10;
        proc->fs = 0x10;
        proc->gs = 0x10;
        proc->ss = 0x10;
        
        proc->eflags = 0;
    }
    
    // 初始化文件描述符
    for (int i = 0; i < 16; i++) {
        proc->fd[i] = -1;
    }
    
    // 标准输入输出错误
    proc->fd[0] = 0;  // stdin
    proc->fd[1] = 1;  // stdout
    proc->fd[2] = 2;  // stderr
    
    // 调度信息
    proc->priority = 10;
    proc->time_slice = 100;  // 100ms
    
    // 添加到进程列表
    proc->next = process_list;
    process_list = proc;
    
    return proc;
}

// 复制当前进程（fork）
pcb_t* fork_process(void) {
    pcb_t* parent = current_process;
    
    // 创建子进程PCB
    pcb_t* child = create_process(parent->name, NULL);
    if (!child) {
        return NULL;
    }
    
    // 设置父子关系
    child->parent = parent;
    child->sibling = parent->children;
    parent->children = child;
    
    // 复制寄存器上下文
    child->eax = parent->eax;
    child->ebx = parent->ebx;
    child->ecx = parent->ecx;
    child->edx = parent->edx;
    child->esi = parent->esi;
    child->edi = parent->edi;
    child->esp = parent->esp;
    child->ebp = parent->ebp;
    child->eip = parent->eip;
    child->eflags = parent->eflags;
    
    child->cs = parent->cs;
    child->ds = parent->ds;
    child->es = parent->es;
    child->fs = parent->fs;
    child->gs = parent->gs;
    child->ss = parent->ss;
    
    // 复制页目录
    // 这里需要复制用户空间的页面
    // 简化实现：只复制内核部分
    
    // 复制文件描述符
    for (int i = 0; i < 16; i++) {
        child->fd[i] = parent->fd[i];
    }
    
    // fork返回值：子进程返回0，父进程返回子进程PID
    parent->eax = child->pid;
    child->eax = 0;
    
    // 设置状态
    child->state = PROCESS_READY;
    
    // 添加到就绪队列
    add_to_ready_queue(child);
    
    return child;
}

// 终止进程
void exit_process(int exit_code) {
    pcb_t* proc = current_process;
    
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr("Process ");
    screen_putstr(proc->name);
    screen_putstr(" (PID=");
    screen_putdec(proc->pid);
    screen_putstr(") exited with code ");
    screen_putdec(exit_code);
    screen_putstr("\n");
    
    // 设置退出状态
    proc->exit_code = exit_code;
    proc->state = PROCESS_ZOMBIE;
    
    // 唤醒父进程
    if (proc->parent) {
        wakeup_process(proc->parent);
    }
    
    // 切换到空闲进程
    current_process = init_process;
    
    // 调度
    schedule();
}

// 等待子进程
void wait_process(uint32_t pid) {
    pcb_t* parent = current_process;
    pcb_t* child = parent->children;
    
    while (child) {
        if ((pid == 0 || child->pid == pid) && 
            child->state == PROCESS_ZOMBIE) {
            // 找到僵尸子进程
            
            // 从子进程列表中移除
            if (parent->children == child) {
                parent->children = child->sibling;
            } else {
                pcb_t* prev = parent->children;
                while (prev->sibling != child) {
                    prev = prev->sibling;
                }
                prev->sibling = child->sibling;
            }
            
            // 释放资源
            kfree(child->page_dir);
            kfree(child);
            
            return;
        }
        child = child->sibling;
    }
    
    // 没有可回收的子进程，阻塞当前进程
    parent->state = PROCESS_BLOCKED;
    schedule();
}

// 杀死进程
void kill_process(uint32_t pid, int signal) {
    pcb_t* proc = process_list;
    
    while (proc) {
        if (proc->pid == pid) {
            screen_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
            screen_putstr("Killing process ");
            screen_putstr(proc->name);
            screen_putstr(" (PID=");
            screen_putdec(pid);
            screen_putstr(") with signal ");
            screen_putdec(signal);
            screen_putstr("\n");
            
            // 发送信号
            // 简化实现：直接终止进程
            exit_process(128 + signal);
            return;
        }
        proc = proc->next;
    }
    
    screen_setcolor(COLOR_YELLOW, COLOR_BLACK);
    screen_putstr("Process ");
    screen_putdec(pid);
    screen_putstr(" not found\n");
}

// 获取当前进程
pcb_t* get_current_process(void) {
    return current_process;
}

// 获取当前进程ID
uint32_t get_pid(void) {
    return current_process ? current_process->pid : 0;
}

// 切换进程上下文
void switch_process(pcb_t* new_process) {
    if (!new_process || new_process == current_process) {
        return;
    }
    
    pcb_t* old_process = current_process;
    
    // 保存旧进程上下文
    if (old_process && old_process->state == PROCESS_RUNNING) {
        // 保存寄存器
        asm volatile("mov %%eax, %0" : "=m"(old_process->eax));
        asm volatile("mov %%ebx, %0" : "=m"(old_process->ebx));
        asm volatile("mov %%ecx, %0" : "=m"(old_process->ecx));
        asm volatile("mov %%edx, %0" : "=m"(old_process->edx));
        asm volatile("mov %%esi, %0" : "=m"(old_process->esi));
        asm volatile("mov %%edi, %0" : "=m"(old_process->edi));
        asm volatile("mov %%esp, %0" : "=m"(old_process->esp));
        asm volatile("mov %%ebp, %0" : "=m"(old_process->ebp));
        
        // 保存eip
        uint32_t eip;
        asm volatile("call 1f\n1: pop %0" : "=r"(eip));
        old_process->eip = eip - 5;  // 减去call指令大小
        
        // 保存eflags
        asm volatile("pushf\npop %0" : "=r"(old_process->eflags));
        
        // 保存段寄存器
        asm volatile("mov %%cs, %0" : "=r"(old_process->cs));
        asm volatile("mov %%ds, %0" : "=r"(old_process->ds));
        asm volatile("mov %%es, %0" : "=r"(old_process->es));
        asm volatile("mov %%fs, %0" : "=r"(old_process->fs));
        asm volatile("mov %%gs, %0" : "=r"(old_process->gs));
        asm volatile("mov %%ss, %0" : "=r"(old_process->ss));
        
        // 更新状态
        old_process->state = PROCESS_READY;
    }
    
    // 设置新进程
    current_process = new_process;
    new_process->state = PROCESS_RUNNING;
    
    // 切换页目录
    if (new_process->page_dir != get_kernel_page_dir()) {
        switch_page_directory(new_process->page_dir);
    }
    
    // 恢复寄存器上下文
    asm volatile("mov %0, %%cr3" : : "r"(new_process->page_dir));
    
    // 设置段寄存器
    asm volatile("mov %0, %%ds" : : "r"(new_process->ds));
    asm volatile("mov %0, %%es" : : "r"(new_process->es));
    asm volatile("mov %0, %%fs" : : "r"(new_process->fs));
    asm volatile("mov %0, %%gs" : : "r"(new_process->gs));
    asm volatile("mov %0, %%ss" : : "r"(new_process->ss));
    
    // 设置栈指针
    asm volatile("mov %0, %%esp" : : "r"(new_process->esp));
    asm volatile("mov %0, %%ebp" : : "r"(new_process->ebp));
    
    // 设置通用寄存器
    asm volatile("mov %0, %%eax" : : "r"(new_process->eax));
    asm volatile("mov %0, %%ebx" : : "r"(new_process->ebx));
    asm volatile("mov %0, %%ecx" : : "r"(new_process->ecx));
    asm volatile("mov %0, %%edx" : : "r"(new_process->edx));
    asm volatile("mov %0, %%esi" : : "r"(new_process->esi));
    asm volatile("mov %0, %%edi" : : "r"(new_process->edi));
    
    // 设置eflags
    asm volatile("push %0\npopf" : : "r"(new_process->eflags));
    
    // 跳转到eip
    asm volatile("jmp *%0" : : "r"(new_process->eip));
}

// 睡眠进程
void sleep_process(uint32_t milliseconds) {
    pcb_t* proc = current_process;
    
    proc->state = PROCESS_BLOCKED;
    proc->wakeup_time = get_ticks() + milliseconds;
    
    schedule();
}

// 唤醒进程
void wakeup_process(pcb_t* process) {
    if (!process || process->state != PROCESS_BLOCKED) {
        return;
    }
    
    process->state = PROCESS_READY;
    add_to_ready_queue(process);
}

// 显示进程信息
void show_processes(void) {
    screen_setcolor(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_putstr("\nProcess List:\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    
    screen_putstr("PID  State       Name\n");
    screen_putstr("---  ----------  --------------------\n");
    
    pcb_t* proc = process_list;
    while (proc) {
        // 显示PID
        screen_putdec(proc->pid);
        if (proc->pid < 10) screen_putstr("  ");
        else if (proc->pid < 100) screen_putstr(" ");
        
        screen_putstr("  ");
        
        // 显示状态
        switch (proc->state) {
            case PROCESS_NEW:      screen_putstr("NEW       "); break;
            case PROCESS_READY:    screen_putstr("READY     "); break;
            case PROCESS_RUNNING:  screen_putstr("RUNNING   "); break;
            case PROCESS_BLOCKED:  screen_putstr("BLOCKED   "); break;
            case PROCESS_ZOMBIE:   screen_putstr("ZOMBIE    "); break;
            case PROCESS_TERMINATED: screen_putstr("TERMINATED"); break;
        }
        
        screen_putstr("  ");
        
        // 显示进程名
        screen_putstr(proc->name);
        
        // 标记当前进程
        if (proc == current_process) {
            screen_putstr("  <--");
        }
        
        screen_putstr("\n");
        
        proc = proc->next;
    }
}