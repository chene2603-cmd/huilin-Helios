#include "system.h"
#include "screen.h"
#include "syscall.h"
#include "process.h"
#include "string.h"

// 简单的内核级控制台循环（替代原来的 console_process 占位）
void console_process(void) {
    char cmd[128];
    int  pos = 0;

    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr("\nHelios console ready. Type 'help' for commands.\n");

    while (1) {
        // 显示提示符
        screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
        screen_putstr("helios> ");
        screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);

        pos = 0;
        memset(cmd, 0, sizeof(cmd));

        // 逐字符读取键盘（使用系统调用 read，fd=0）
        while (1) {
            char c;
            int n = sys_read(0, &c, 1);   // 在你现有的 sys_read 中，fd=0 会轮询 0x60 键盘
            if (n <= 0)
                continue;

            // 回车处理
            if (c == '\n') {
                cmd[pos] = '\0';
                screen_putchar('\n');
                break;
            }
            // 退格处理
            else if (c == 0x08 || c == 0x7F) {
                if (pos > 0) {
                    pos--;
                    // 退格显示
                    screen_putchar('\b');
                    screen_putchar(' ');
                    screen_putchar('\b');
                }
            }
            // 普通字符
            else if (pos < 126) {
                cmd[pos++] = c;
                screen_putchar(c);
            }
        }

        // 命令解析
        if (strcmp(cmd, "help") == 0) {
            screen_putstr("Available commands:\n");
            screen_putstr("  help   - show this help\n");
            screen_putstr("  clear  - clear screen\n");
            screen_putstr("  exec   - run /bin/init (if loaded)\n");
        }
        else if (strcmp(cmd, "clear") == 0) {
            screen_clear();
        }
        else if (strcmp(cmd, "exec") == 0) {
            // 尝试用 execve 执行 /bin/init
            char* argv[] = {"/bin/init", NULL};
            int ret = sys_execve("/bin/init", argv, NULL);
            if (ret != 0) {
                screen_setcolor(COLOR_RED, COLOR_BLACK);
                screen_putstr("Exec failed.\n");
                screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
            }
        }
        else if (strlen(cmd) > 0) {
            screen_putstr("Unknown command: ");
            screen_putstr(cmd);
            screen_putstr("\n");
        }
    }
}