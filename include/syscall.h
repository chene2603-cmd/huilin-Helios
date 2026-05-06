#ifndef SYSCALL_H
#define SYSCALL_H

#include "system.h"
#include "process.h"

// 系统调用号
#define SYS_EXIT    0
#define SYS_FORK    1
#define SYS_READ    2
#define SYS_WRITE   3
#define SYS_OPEN    4
#define SYS_CLOSE   5
#define SYS_EXECVE  6
#define SYS_WAITPID 7
#define SYS_BRK     8
#define SYS_GETPID  9
#define SYS_SLEEP   10
#define SYS_GETTIME 11
#define SYS_YIELD   12
#define SYS_SBRK    13
#define SYS_KILL    14
#define SYS_DUP     15
#define SYS_PIPE    16
#define SYS_CHDIR   17
#define SYS_GETCWD  18
#define SYS_STAT    19

// 系统调用最大数量
#define MAX_SYSCALLS 32

// 系统调用函数类型
typedef int32_t (*syscall_t)(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

// 系统调用信息
typedef struct {
    uint32_t num;
    const char* name;
    syscall_t handler;
} syscall_info_t;

// 系统调用函数声明
void init_syscalls(void);
void syscall_handler(registers_t* regs);
int32_t register_syscall(uint32_t num, const char* name, syscall_t handler);
syscall_info_t* get_syscall_info(uint32_t num);

// 系统调用实现
int32_t sys_exit(int exit_code);
int32_t sys_fork(void);
int32_t sys_read(int fd, void* buf, uint32_t count);
int32_t sys_write(int fd, const void* buf, uint32_t count);
int32_t sys_open(const char* path, int flags, int mode);
int32_t sys_close(int fd);
int32_t sys_execve(const char* path, char* const argv[], char* const envp[]);
int32_t sys_waitpid(int pid, int* status, int options);
int32_t sys_brk(void* addr);
int32_t sys_getpid(void);
int32_t sys_sleep(uint32_t milliseconds);
int32_t sys_gettime(void);
int32_t sys_yield(void);
int32_t sys_sbrk(int32_t increment);
int32_t sys_kill(int pid, int sig);
int32_t sys_dup(int oldfd);
int32_t sys_pipe(int pipefd[2]);
int32_t sys_chdir(const char* path);
int32_t sys_getcwd(char* buf, uint32_t size);
int32_t sys_stat(const char* path, void* statbuf);

#endif