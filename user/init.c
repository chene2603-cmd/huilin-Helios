// user/init.c
// 第一个用户进程
#include "../include/syscall.h"

void _start(void) {
    sys_write(1, "Init process is running!\n", 25);
    
    while (1) {
        sys_yield();   // 让出CPU
    }
}