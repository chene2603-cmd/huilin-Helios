#include "system.h"
#include "syscall.h"
#include "string.h"

// 测试程序入口
void _start(void) {
    sys_write(1, "\n=== System Test Program ===\n", 28);
    
    // 测试系统调用
    sys_write(1, "1. Testing getpid()... ", 23);
    int pid = sys_getpid();
    char buf[32];
    int len = 0;
    int n = pid;
    do {
        buf[len++] = '0' + (n % 10);
        n /= 10;
    } while (n > 0);
    
    sys_write(1, "PID=", 4);
    for (int i = len - 1; i >= 0; i--) {
        sys_write(1, &buf[i], 1);
    }
    sys_write(1, "\n", 1);
    
    // 测试fork
    sys_write(1, "2. Testing fork()...\n", 21);
    int child = sys_fork();
    
    if (child == 0) {
        // 子进程
        sys_write(1, "  Child process running\n", 24);
        sys_write(1, "  Child PID: ", 13);
        
        int pid = sys_getpid();
        len = 0;
        n = pid;
        do {
            buf[len++] = '0' + (n % 10);
            n /= 10;
        } while (n > 0);
        
        for (int i = len - 1; i >= 0; i--) {
            sys_write(1, &buf[i], 1);
        }
        sys_write(1, "\n", 1);
        
        sys_write(1, "  Child exiting\n", 16);
        sys_exit(0);
    } else if (child > 0) {
        // 父进程
        sys_write(1, "  Parent waiting for child...\n", 30);
        int status;
        sys_waitpid(child, &status, 0);
        sys_write(1, "  Child exited with status: ", 28);
        
        len = 0;
        n = status;
        do {
            buf[len++] = '0' + (n % 10);
            n /= 10;
        } while (n > 0);
        
        for (int i = len - 1; i >= 0; i--) {
            sys_write(1, &buf[i], 1);
        }
        sys_write(1, "\n", 1);
    } else {
        sys_write(1, "  Fork failed\n", 14);
    }
    
    // 测试sleep
    sys_write(1, "3. Testing sleep()...\n", 22);
    int start = sys_gettime();
    sys_sleep(1000);  // 1秒
    int end = sys_gettime();
    
    sys_write(1, "  Slept for ", 12);
    len = 0;
    n = end - start;
    do {
        buf[len++] = '0' + (n % 10);
        n /= 10;
    } while (n > 0);
    
    for (int i = len - 1; i >= 0; i--) {
        sys_write(1, &buf[i], 1);
    }
    sys_write(1, " ms\n", 4);
    
    // 测试文件操作
    sys_write(1, "4. Testing file I/O...\n", 23);
    int fd = sys_open("/dev/tty", O_RDWR, 0);
    if (fd >= 0) {
        sys_write(1, "  Opened /dev/tty, fd=", 22);
        
        len = 0;
        n = fd;
        do {
            buf[len++] = '0' + (n % 10);
            n /= 10;
        } while (n > 0);
        
        for (int i = len - 1; i >= 0; i--) {
            sys_write(1, &buf[i], 1);
        }
        sys_write(1, "\n", 1);
        
        char msg[] = "  Hello from test program!\n";
        sys_write(fd, msg, sizeof(msg) - 1);
        
        sys_close(fd);
        sys_write(1, "  Closed file\n", 14);
    } else {
        sys_write(1, "  Failed to open file\n", 22);
    }
    
    // 测试内存分配
    sys_write(1, "5. Testing memory allocation...\n", 32);
    int* ptr = (int*)sys_sbrk(1024);
    if (ptr != (void*)-1) {
        sys_write(1, "  Allocated 1KB at ", 19);
        
        // 显示地址
        uint32_t addr = (uint32_t)ptr;
        char hex[] = "0123456789ABCDEF";
        for (int i = 7; i >= 0; i--) {
            sys_write(1, &hex[(addr >> (i * 4)) & 0xF], 1);
        }
        sys_write(1, "\n", 1);
        
        // 测试写入
        for (int i = 0; i < 256; i++) {
            ptr[i] = i;
        }
        
        // 测试读取
        int ok = 1;
        for (int i = 0; i < 256; i++) {
            if (ptr[i] != i) {
                ok = 0;
                break;
            }
        }
        
        if (ok) {
            sys_write(1, "  Memory test passed\n", 21);
        } else {
            sys_write(1, "  Memory test failed\n", 21);
        }
    } else {
        sys_write(1, "  sbrk failed\n", 14);
    }
    
    sys_write(1, "\n=== All tests completed ===\n", 29);
    sys_exit(0);
}