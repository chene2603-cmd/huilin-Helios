#include "system.h"
#include "screen.h"
#include "scheduler.h"
#include "process.h"
#include "timer.h"

// 调度队列
static pcb_t* ready_queue = NULL;
static pcb_t* ready_queue_tail = NULL;
static pcb_t* blocked_queue = NULL;

// 调度器初始化
void init_scheduler(void) {
    screen_putstr("Initializing Scheduler... ");
    
    ready_queue = NULL;
    ready_queue_tail = NULL;
    blocked_queue = NULL;
    
    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("OK\n");
}

// 添加到就绪队列
void add_to_ready_queue(pcb_t* process) {
    if (!process) return;
    
    process->next = NULL;
    
    if (!ready_queue) {
        ready_queue = process;
        ready_queue_tail = process;
    } else {
        ready_queue_tail->next = process;
        ready_queue_tail = process;
    }
}

// 从就绪队列移除
void remove_from_ready_queue(pcb_t* process) {
    if (!process || !ready_queue) return;
    
    if (ready_queue == process) {
        ready_queue = process->next;
        if (ready_queue_tail == process) {
            ready_queue_tail = NULL;
        }
    } else {
        pcb_t* prev = ready_queue;
        while (prev->next && prev->next != process) {
            prev = prev->next;
        }
        
        if (prev->next == process) {
            prev->next = process->next;
            if (ready_queue_tail == process) {
                ready_queue_tail = prev;
            }
        }
    }
    
    process->next = NULL;
}

// 调度函数
void schedule(void) {
    static uint32_t ticks = 0;
    pcb_t* current = get_current_process();
    
    // 检查时间片
    if (current && current != init_process) {
        if (current->time_slice > 0) {
            current->time_slice--;
            return;
        }
    }
    
    // 重置时间片
    if (current) {
        current->time_slice = 100;  // 100ms
    }
    
    // 选择下一个进程
    pcb_t* next = ready_queue;
    if (!next) {
        // 没有就绪进程，切换到空闲进程
        next = init_process;
    }
    
    // 从队列中移除
    if (next != init_process) {
        remove_from_ready_queue(next);
    }
    
    // 将当前进程放回队列
    if (current && current != init_process && 
        current->state == PROCESS_RUNNING) {
        current->state = PROCESS_READY;
        add_to_ready_queue(current);
    }
    
    // 切换到下一个进程
    if (next != current) {
        switch_process(next);
    }
}

// 主动让出CPU
void yield(void) {
    pcb_t* current = get_current_process();
    
    if (current && current != init_process) {
        current->state = PROCESS_READY;
        add_to_ready_queue(current);
    }
    
    schedule();
}

// 定时器中断处理
void timer_tick_handler(registers_t* regs) {
    static uint32_t tick_count = 0;
    
    tick_count++;
    
    // 更新系统时间
    update_ticks();
    
    // 检查阻塞进程
    pcb_t* proc = blocked_queue;
    pcb_t* prev = NULL;
    
    while (proc) {
        if (proc->state == PROCESS_BLOCKED && 
            proc->wakeup_time <= tick_count) {
            // 唤醒进程
            pcb_t* to_wake = proc;
            
            if (prev) {
                prev->next = proc->next;
            } else {
                blocked_queue = proc->next;
            }
            
            to_wake->state = PROCESS_READY;
            add_to_ready_queue(to_wake);
            
            proc = prev ? prev->next : blocked_queue;
        } else {
            prev = proc;
            proc = proc->next;
        }
    }
    
    // 调度
    schedule();
}

// 设置进程优先级
void set_process_priority(pcb_t* process, uint32_t priority) {
    if (!process) return;
    
    process->priority = priority;
    
    // 重新计算时间片
    process->time_slice = priority * 10;
}

// 显示调度信息
void show_scheduler_info(void) {
    screen_setcolor(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_putstr("\nScheduler Information:\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    
    // 就绪队列长度
    uint32_t ready_count = 0;
    pcb_t* proc = ready_queue;
    while (proc) {
        ready_count++;
        proc = proc->next;
    }
    
    // 阻塞队列长度
    uint32_t blocked_count = 0;
    proc = blocked_queue;
    while (proc) {
        blocked_count++;
        proc = proc->next;
    }
    
    screen_putstr("  Ready queue: ");
    screen_putdec(ready_count);
    screen_putstr(" processes\n");
    screen_putstr("  Blocked queue: ");
    screen_putdec(blocked_count);
    screen_putstr(" processes\n");
    
    screen_putstr("  Current process: ");
    pcb_t* current = get_current_process();
    if (current) {
        screen_putstr(current->name);
        screen_putstr(" (PID=");
        screen_putdec(current->pid);
        screen_putstr(")\n");
    } else {
        screen_putstr("None\n");
    }
    
    // 显示就绪队列
    if (ready_queue) {
        screen_putstr("\n  Ready queue contents:\n");
        proc = ready_queue;
        while (proc) {
            screen_putstr("    ");
            screen_putstr(proc->name);
            screen_putstr(" (PID=");
            screen_putdec(proc->pid);
            screen_putstr(", Priority=");
            screen_putdec(proc->priority);
            screen_putstr(")\n");
            proc = proc->next;
        }
    }
}