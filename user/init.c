#include "system.h"
#include "syscall.h"

// 初始化进程入口
void _start(void) {
    // 系统调用测试
    sys_write(1, "Init process started\n", 21);
    
    // 创建shell进程
    int pid = sys_fork();
    if (pid == 0) {
        // 子进程：运行shell
        char* argv[] = {"/bin/sh", NULL};
        sys_execve("/bin/sh", argv, NULL);
        
        // 如果execve失败
        sys_write(2, "Failed to execute shell\n", 24);
        sys_exit(1);
    } else if (pid > 0) {
        // 父进程：等待shell退出
        sys_write(1, "Shell PID: ", 11);
        
        char buf[16];
        int len = 0;
        int n = pid;
        do {
            buf[len++] = '0' + (n % 10);
            n /= 10;
        } while (n > 0);
        
        for (int i = len - 1; i >= 0; i--) {
            sys_write(1, &buf[i], 1);
        }
        sys_write(1, "\n", 1);
        
        int status;
        sys_waitpid(pid, &status, 0);
        
        sys_write(1, "Shell exited\n", 13);
    } else {
        sys_write(2, "Failed to fork\n", 15);
    }
    
    // 空闲循环
    while (1) {
        sys_yield();
    }
}