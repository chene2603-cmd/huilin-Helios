#include "system.h"
#include "screen.h"
#include "syscall.h"
#include "process.h"
#include "timer.h"
#include "string.h"
#include "isr.h"
#include "pmm.h"
#include "vmm.h"
#include "paging.h"

// 系统调用表
static syscall_info_t syscall_table[MAX_SYSCALLS];
static uint32_t syscall_count = 0;

// ---------- 极简 ramdisk ----------
#define RAMDISK_MAX 8
static struct {
    const char *name;
    uint8_t   *data;
    uint32_t   size;
} ramdisk[RAMDISK_MAX];

// 注册用户程序到 ramdisk
static int ramdisk_register(const char *name, uint8_t *code, uint32_t size) {
    for (int i = 0; i < RAMDISK_MAX; i++) {
        if (ramdisk[i].name && strcmp(ramdisk[i].name, name) == 0) {
            ramdisk[i].data = code;
            ramdisk[i].size = size;
            return 0;
        }
        if (!ramdisk[i].name) {
            ramdisk[i].name = name;
            ramdisk[i].data = code;
            ramdisk[i].size = size;
            return 0;
        }
    }
    return -1;
}

// 从 ramdisk 加载二进制到用户空间，返回入口和栈顶
static int ramdisk_load(const char *path, void **entry, void **stack_top) {
    for (int i = 0; i < RAMDISK_MAX && ramdisk[i].name; i++) {
        if (strcmp(ramdisk[i].name, path) == 0) {
            if (!ramdisk[i].data) return -1;

            void *user_base = (void*)0x400000;
            uint8_t *code = ramdisk[i].data;
            uint32_t size = ramdisk[i].size;

            // 映射代码页
            for (uint32_t off = 0; off < size; off += 0x1000) {
                void *vaddr = user_base + off;
                void *page = pmm_alloc_page();
                if (!page) return -1;
                vmm_map_page(vaddr, page, PAGE_USER | PAGE_PRESENT | PAGE_RW);
                uint32_t chunk = (size - off > 0x1000) ? 0x1000 : (size - off);
                memcpy(vaddr, code + off, chunk);
            }

            // 用户栈
            void *stack_vaddr = (void*)0xBFC00000;
            void *stack_page = pmm_alloc_page();
            if (!stack_page) return -1;
            vmm_map_page(stack_vaddr - 0x1000, stack_page,
                         PAGE_USER | PAGE_PRESENT | PAGE_RW);

            *entry = user_base;
            *stack_top = stack_vaddr;
            return 0;
        }
    }
    return -1;
}

// 初始化系统调用
void init_syscalls(void) {
    screen_putstr("Initializing System Calls... ");

    memset(syscall_table, 0, sizeof(syscall_table));
    syscall_count = 0;

    register_syscall(SYS_EXIT,    "exit",    (syscall_t)sys_exit);
    register_syscall(SYS_FORK,    "fork",    (syscall_t)sys_fork);
    register_syscall(SYS_READ,    "read",    (syscall_t)sys_read);
    register_syscall(SYS_WRITE,   "write",   (syscall_t)sys_write);
    register_syscall(SYS_OPEN,    "open",    (syscall_t)sys_open);
    register_syscall(SYS_CLOSE,   "close",   (syscall_t)sys_close);
    register_syscall(SYS_EXECVE,  "execve",  (syscall_t)sys_execve);
    register_syscall(SYS_WAITPID, "waitpid", (syscall_t)sys_waitpid);
    register_syscall(SYS_BRK,     "brk",     (syscall_t)sys_brk);
    register_syscall(SYS_GETPID,  "getpid",  (syscall_t)sys_getpid);
    register_syscall(SYS_SLEEP,   "sleep",   (syscall_t)sys_sleep);
    register_syscall(SYS_GETTIME, "gettime", (syscall_t)sys_gettime);
    register_syscall(SYS_YIELD,   "yield",   (syscall_t)sys_yield);
    register_syscall(SYS_SBRK,    "sbrk",    (syscall_t)sys_sbrk);
    register_syscall(SYS_KILL,    "kill",    (syscall_t)sys_kill);

    register_interrupt_handler(0x80, syscall_handler);

    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("OK\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr("  Registered ");
    screen_putdec(syscall_count);
    screen_putstr(" system calls\n");
}

// 系统调用处理程序
void syscall_handler(registers_t* regs) {
    uint32_t num = regs->eax;

    if (num >= MAX_SYSCALLS || !syscall_table[num].handler) {
        regs->eax = -1;
        return;
    }

    syscall_t handler = syscall_table[num].handler;
    int32_t result = handler(regs->ebx, regs->ecx, regs->edx,
                             regs->esi, regs->edi);
    regs->eax = result;
}

int32_t register_syscall(uint32_t num, const char* name, syscall_t handler) {
    if (num >= MAX_SYSCALLS || syscall_table[num].handler)
        return -1;
    syscall_table[num].num = num;
    syscall_table[num].name = name;
    syscall_table[num].handler = handler;
    syscall_count++;
    return 0;
}

syscall_info_t* get_syscall_info(uint32_t num) {
    if (num >= MAX_SYSCALLS) return NULL;
    return &syscall_table[num];
}

// ========== 系统调用实现 ==========

int32_t sys_exit(int exit_code) {
    exit_process(exit_code);
    return 0;
}

int32_t sys_fork(void) {
    pcb_t* child = fork_process();
    return child ? child->pid : -1;
}

int32_t sys_read(int fd, void* buf, uint32_t count) {
    if (fd == 0) {
        char* buffer = (char*)buf;
        uint32_t i = 0;
        while (i < count) {
            uint8_t key = inb(0x60);
            if (key < 0x80) {
                char c = 0;
                switch (key) {
                    case 0x1C: c = '\n'; break;
                    case 0x0E:
                        if (i > 0) { i--; buffer[i] = '\0'; }
                        break;
                    case 0x39: c = ' '; break;
                    default:
                        if (key >= 0x10 && key <= 0x1C)
                            c = "qwertyuiop"[key - 0x10];
                        else if (key >= 0x1E && key <= 0x26)
                            c = "asdfghjkl"[key - 0x1E];
                        else if (key >= 0x2C && key <= 0x32)
                            c = "zxcvbnm"[key - 0x2C];
                        else if (key >= 0x02 && key <= 0x0B)
                            c = "1234567890-="[key - 0x02];
                }
                if (c && i < count) {
                    buffer[i++] = c;
                    screen_putchar(c);
                    if (c == '\n') break;
                }
            }
            for (volatile int j = 0; j < 1000; j++);
        }
        return i;
    }
    return -1;
}

int32_t sys_write(int fd, const void* buf, uint32_t count) {
    if (fd == 1 || fd == 2) {
        const char* buffer = (const char*)buf;
        for (uint32_t i = 0; i < count; i++)
            screen_putchar(buffer[i]);
        return count;
    }
    return -1;
}

int32_t sys_open(const char* path, int flags, int mode) {
    if (strcmp(path, "/dev/tty") == 0 ||
        strcmp(path, "/dev/console") == 0) return 0;
    if (strcmp(path, "/dev/stdout") == 0) return 1;
    if (strcmp(path, "/dev/stderr") == 0) return 2;
    return -1;
}

int32_t sys_close(int fd) {
    pcb_t* proc = get_current_process();
    if (fd < 0 || fd >= 16) return -1;
    proc->fd[fd] = -1;
    return 0;
}

int32_t sys_execve(const char* path, char* const argv[], char* const envp[]) {
    void *entry, *stack_top;
    if (ramdisk_load(path, &entry, &stack_top) != 0) {
        screen_setcolor(COLOR_RED, COLOR_BLACK);
        screen_putstr("execve: not found: ");
        screen_putstr(path);
        screen_putstr("\n");
        return -1;
    }

    pcb_t* cur = get_current_process();
    cur->eip = (uint32_t)entry;
    cur->esp = (uint32_t)stack_top;
    return 0;
}

int32_t sys_waitpid(int pid, int* status, int options) {
    wait_process(pid);
    if (status) *status = 0;
    return pid;
}

int32_t sys_brk(void* addr) {
    pcb_t* proc = get_current_process();
    if (!addr) return proc->brk;
    proc->brk = (uint32_t)addr;
    return 0;
}

int32_t sys_getpid(void) {
    return get_pid();
}

int32_t sys_sleep(uint32_t milliseconds) {
    sleep_process(milliseconds);
    return 0;
}

int32_t sys_gettime(void) {
    return get_time_ms();
}

int32_t sys_yield(void) {
    yield();
    return 0;
}

int32_t sys_sbrk(int32_t increment) {
    pcb_t* proc = get_current_process();
    uint32_t old = proc->brk;
    proc->brk += increment;
    return old;
}

int32_t sys_kill(int pid, int sig) {
    kill_process(pid, sig);
    return 0;
}

int32_t sys_dup(int oldfd) {
    pcb_t* proc = get_current_process();
    if (oldfd < 0 || oldfd >= 16 || proc->fd[oldfd] == -1)
        return -1;
    for (int i = 3; i < 16; i++) {
        if (proc->fd[i] == -1) {
            proc->fd[i] = proc->fd[oldfd];
            return i;
        }
    }
    return -1;
}

int32_t sys_pipe(int pipefd[2]) {
    pcb_t* proc = get_current_process();
    int fd1 = -1, fd2 = -1;
    for (int i = 3; i < 16; i++) {
        if (proc->fd[i] == -1) {
            if (fd1 == -1) fd1 = i;
            else { fd2 = i; break; }
        }
    }
    if (fd1 == -1 || fd2 == -1) return -1;
    proc->fd[fd1] = 100;
    proc->fd[fd2] = 101;
    if (pipefd) {
        pipefd[0] = fd1;
        pipefd[1] = fd2;
    }
    return 0;
}

int32_t sys_chdir(const char* path) {
    (void)path;
    return 0;
}

int32_t sys_getcwd(char* buf, uint32_t size) {
    if (!buf || size < 2) return -1;
    buf[0] = '/';
    buf[1] = '\0';
    return 1;
}

int32_t sys_stat(const char* path, void* statbuf) {
    if (!statbuf) return -1;
    struct stat* st = (struct stat*)statbuf;
    memset(st, 0, sizeof(struct stat));
    st->st_mode = S_IFREG | 0644;
    st->st_size = 0;
    return 0;
}

void show_syscalls(void) {
    screen_setcolor(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_putstr("\nSystem Call Table:\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr("Num  Name\n---  -----\n");
    for (uint32_t i = 0; i < MAX_SYSCALLS; i++) {
        if (syscall_table[i].handler) {
            screen_putdec(i);
            if (i < 10) screen_putstr("   ");
            else if (i < 100) screen_putstr("  ");
            else screen_putstr(" ");
            screen_putstr("  ");
            screen_putstr(syscall_table[i].name);
            screen_putstr("\n");
        }
    }
}