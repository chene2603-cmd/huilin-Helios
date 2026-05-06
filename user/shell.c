// user/shell.c
// 用户态 shell（简化版）
#include "../include/syscall.h"

void _start(void) {
    char buf[2];
    sys_write(1, "Shell> ", 7);
    
    while (1) {
        int n = sys_read(0, buf, 1);
        if (n > 0) {
            sys_write(1, buf, 1);
            if (buf[0] == '\n') {
                sys_write(1, "Shell> ", 7);
            }
        }
        sys_yield();
    }
}