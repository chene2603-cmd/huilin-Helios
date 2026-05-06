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
static pcb_t *ready_queue = NULL;

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

pcb_t *scheduler_get_next(void) {
    if (current_process && current_process->state == PROCESS_RUNNING) {
        current_process->state = PROCESS_READY;
        enqueue_process(current_process);
    }
    pcb_t *next = dequeue_process();
    if (!next) {
        screen_putstr("No ready process!\n");
        for(;;);
    }
    next->state = PROCESS_RUNNING;
    current_process = next;
    return next;
}

pcb_t *get_current_process(void) {
    return current_process;
}

int32_t get_pid(void) {
    if (!current_process) return 0;
    return current_process->pid;
}

void init_process(void) {
    screen_putstr("Initializing Process Subsystem... ");
    memset(process_table, 0, sizeof(process_table));
    ready_queue = NULL;

    pcb_t *idle = &process_table[0];
    idle->pid = 0;
    strcpy(idle->name, "idle");
    idle->state = PROCESS_RUNNING;
    idle->page_dir = vmm_get_kernel_directory();
    idle->brk = 0x100000;
    idle->time_slice = 10;
    for (int i = 0; i < 16; i++) idle->fd[i] = -1;
    current_process = idle;
    next_pid = 1;

    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("OK\n");
}

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

static page_directory_t *clone_user_space(page_directory_t *src) {
    // 简化：直接共享页目录（真实系统应做 COW）
    return src;
}

pcb_t *fork_process(void) {
    if (!current_process) return NULL;

    pcb_t *child = create_process_empty();
    if (!child) return NULL;

    strcpy(child->name, current_process->name);
    child->parent = current_process;
    child->page_dir = clone_user_space(current_process->page_dir);
    child->brk = current_process->brk;

    for (int i = 0; i < 16; i++) child->fd[i] = current_process->fd[i];

    child->eip = current_process->eip;
    child->esp = current_process->esp;
    child->ebp = current_process->ebp;
    child->eax = 0;
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

    return child;
}

void exit_process(int exit_code) {
    if (!current_process) return;
    current_process->exit_code = exit_code;
    current_process->state = PROCESS_TERMINATED;
    scheduler_get_next();
}

void wait_process(int pid) {
    (void)pid;
    current_process->state = PROCESS_BLOCKED;
    scheduler_get_next();
}

void sleep_process(uint32_t ms) {
    current_process->sleep_until = get_time_ms() + ms;
    current_process->state = PROCESS_BLOCKED;
    scheduler_get_next();
}

int32_t kill_process(int pid, int sig) {
    for (int i = 0; i < MAX_PROCESSES; i++) {
        if (process_table[i].pid == pid && process_table[i].state != PROCESS_TERMINATED) {
            process_table[i].state = PROCESS_TERMINATED;
            return 0;
        }
    }
    return -1;
}

void yield(void) {
    asm volatile("int $0x20");
}