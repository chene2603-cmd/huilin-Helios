#include "system.h"
#include "screen.h"
#include "console.h"
#include "kheap.h"
#include "string.h"
#include "process.h"
#include "scheduler.h"
#include "timer.h"
#include "syscall.h"
#include "fs.h"
#include "device.h"
#include "ata.h"

// 全局控制台
static console_t console;

// 命令表
static command_t commands[32];
static int command_count = 0;

// 初始化控制台
void init_console(void) {
    screen_putstr("Initializing Console... ");
    
    memset(&console, 0, sizeof(console_t));
    memset(commands, 0, sizeof(commands));
    
    // 注册标准命令
    register_command("help", "Show this help message", cmd_help);
    register_command("clear", "Clear the screen", cmd_clear);
    register_command("echo", "Echo arguments", cmd_echo);
    register_command("mem", "Show memory information", cmd_mem);
    register_command("ps", "List processes", cmd_ps);
    register_command("time", "Show system time", cmd_time);
    register_command("sysinfo", "Show system information", cmd_sysinfo);
    register_command("ls", "List directory", cmd_ls);
    register_command("cat", "Show file content", cmd_cat);
    register_command("test", "Run tests", cmd_test);
    
    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("OK\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr("  Registered ");
    screen_putdec(command_count);
    screen_putstr(" commands\n");
}

// 控制台输出字符
void console_putchar(char c) {
    screen_putchar(c);
    
    // 添加到缓冲区
    if (console.pos < CONSOLE_BUFFER_SIZE - 1) {
        console.buffer[console.pos++] = c;
    }
    
    if (c == '\n') {
        console.buffer[console.pos++] = '\r';
    }
}

// 控制台输出字符串
void console_putstr(const char* str) {
    while (*str) {
        console_putchar(*str++);
    }
}

// 控制台输出十六进制
void console_puthex(uint32_t n) {
    char buf[9];
    for (int i = 7; i >= 0; i--) {
        uint8_t digit = (n >> (i * 4)) & 0xF;
        buf[7 - i] = digit < 10 ? '0' + digit : 'A' + digit - 10;
    }
    buf[8] = '\0';
    console_putstr(buf);
}

// 控制台输出十进制
void console_putdec(uint32_t n) {
    if (n == 0) {
        console_putchar('0');
        return;
    }
    
    char buf[12];
    int i = 0;
    
    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    
    for (int j = i - 1; j >= 0; j--) {
        console_putchar(buf[j]);
    }
}

// 清空控制台
void console_clear(void) {
    screen_clear();
    console.pos = 0;
    console.cursor_x = 0;
    console.cursor_y = 0;
    screen_set_cursor(0, 0);
}

// 处理控制台输入
void console_process(void) {
    uint8_t key = inb(0x60);
    
    if (key < 0x80) {  // 键按下
        char c = 0;
        
        switch (key) {
            case 0x1C:  // 回车
                console_putchar('\n');
                console.buffer[console.pos] = '\0';
                
                // 添加到历史记录
                if (console.pos > 0) {
                    strncpy(console.history[console.history_pos], console.buffer, 255);
                    console.history_pos = (console.history_pos + 1) % HISTORY_SIZE;
                    if (console.history_count < HISTORY_SIZE) {
                        console.history_count++;
                    }
                }
                
                // 执行命令
                console_execute(console.buffer);
                
                // 重置缓冲区
                console.pos = 0;
                console_prompt();
                break;
                
            case 0x0E:  // 退格
                if (console.pos > 0) {
                    console.pos--;
                    console_putchar('\b');
                }
                break;
                
            case 0x48:  // 上箭头
                if (console.history_count > 0) {
                    // 显示上一条历史记录
                    int index = (console.history_pos - 1 + HISTORY_SIZE) % HISTORY_SIZE;
                    console_clear();
                    console_prompt();
                    console_putstr(console.history[index]);
                    strcpy(console.buffer, console.history[index]);
                    console.pos = strlen(console.buffer);
                }
                break;
                
            case 0x50:  // 下箭头
                // 显示下一条历史记录
                if (console.history_count > 0) {
                    int index = (console.history_pos) % HISTORY_SIZE;
                    console_clear();
                    console_prompt();
                    console_putstr(console.history[index]);
                    strcpy(console.buffer, console.history[index]);
                    console.pos = strlen(console.buffer);
                }
                break;
                
            default:
                // 字母和数字
                if (key >= 0x10 && key <= 0x1C) {
                    c = "qwertyuiop"[key - 0x10];
                } else if (key >= 0x1E && key <= 0x26) {
                    c = "asdfghjkl"[key - 0x1E];
                } else if (key >= 0x2C && key <= 0x32) {
                    c = "zxcvbnm"[key - 0x2C];
                } else if (key >= 0x02 && key <= 0x0B) {
                    c = "1234567890-="[key - 0x02];
                } else if (key == 0x39) {
                    c = ' ';
                }
                
                if (c && console.pos < CONSOLE_BUFFER_SIZE - 1) {
                    console.buffer[console.pos++] = c;
                    console_putchar(c);
                }
                break;
        }
    }
}

// 执行命令
void console_execute(const char* cmd) {
    if (strlen(cmd) == 0) {
        return;
    }
    
    execute_command(cmd);
}

// 显示提示符
void console_prompt(void) {
    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    console_putstr("helios");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    console_putstr("> ");
}

// 读取一行
int console_readline(char* buf, uint32_t size) {
    uint32_t pos = 0;
    
    while (1) {
        uint8_t key = inb(0x60);
        
        if (key < 0x80) {  // 键按下
            char c = 0;
            
            switch (key) {
                case 0x1C:  // 回车
                    if (buf && pos < size) {
                        buf[pos] = '\0';
                    }
                    console_putchar('\n');
                    return pos;
                    
                case 0x0E:  // 退格
                    if (pos > 0) {
                        pos--;
                        console_putchar('\b');
                    }
                    break;
                    
                default:
                    if (key >= 0x10 && key <= 0x1C) {
                        c = "qwertyuiop"[key - 0x10];
                    } else if (key >= 0x1E && key <= 0x26) {
                        c = "asdfghjkl"[key - 0x1E];
                    } else if (key >= 0x2C && key <= 0x32) {
                        c = "zxcvbnm"[key - 0x2C];
                    } else if (key >= 0x02 && key <= 0x0B) {
                        c = "1234567890-="[key - 0x02];
                    } else if (key == 0x39) {
                        c = ' ';
                    }
                    
                    if (c && pos < size - 1) {
                        if (buf) {
                            buf[pos++] = c;
                        }
                        console_putchar(c);
                    }
                    break;
            }
        }
    }
}

// 注册命令
int register_command(const char* name, const char* desc, int (*handler)(int, char**)) {
    if (command_count >= 32) {
        return -1;
    }
    
    commands[command_count].name = name;
    commands[command_count].description = desc;
    commands[command_count].handler = handler;
    
    command_count++;
    return 0;
}

// 执行命令
int execute_command(const char* cmd) {
    char* argv[MAX_ARGS];
    int argc = 0;
    
    // 复制命令字符串
    char* cmd_copy = (char*)kmalloc(strlen(cmd) + 1);
    if (!cmd_copy) {
        return -1;
    }
    
    strcpy(cmd_copy, cmd);
    
    // 解析参数
    char* token = strtok(cmd_copy, " ");
    while (token && argc < MAX_ARGS) {
        argv[argc++] = token;
        token = strtok(NULL, " ");
    }
    
    if (argc == 0) {
        kfree(cmd_copy);
        return 0;
    }
    
    // 查找命令
    for (int i = 0; i < command_count; i++) {
        if (strcmp(commands[i].name, argv[0]) == 0) {
            int result = commands[i].handler(argc, argv);
            kfree(cmd_copy);
            return result;
        }
    }
    
    console_putstr("Command not found: ");
    console_putstr(argv[0]);
    console_putstr("\nType 'help' for available commands\n");
    
    kfree(cmd_copy);
    return -1;
}

// 显示帮助
void show_help(int argc, char* argv[]) {
    console_putstr("\nAvailable commands:\n");
    console_putstr("------------------\n");
    
    for (int i = 0; i < command_count; i++) {
        console_putstr("  ");
        console_putstr(commands[i].name);
        
        int len = strlen(commands[i].name);
        for (int j = len; j < 12; j++) {
            console_putstr(" ");
        }
        
        console_putstr("  ");
        console_putstr(commands[i].description);
        console_putstr("\n");
    }
}

// 命令实现
int cmd_help(int argc, char* argv[]) {
    show_help(argc, argv);
    return 0;
}

int cmd_clear(int argc, char* argv[]) {
    console_clear();
    return 0;
}

int cmd_echo(int argc, char* argv[]) {
    for (int i = 1; i < argc; i++) {
        console_putstr(argv[i]);
        console_putstr(" ");
    }
    console_putstr("\n");
    return 0;
}

int cmd_mem(int argc, char* argv[]) {
    show_memory_status();
    return 0;
}

int cmd_ps(int argc, char* argv[]) {
    show_processes();
    return 0;
}

int cmd_time(int argc, char* argv[]) {
    show_time();
    return 0;
}

int cmd_sysinfo(int argc, char* argv[]) {
    screen_setcolor(COLOR_LIGHT_CYAN, COLOR_BLACK);
    console_putstr("\nSystem Information:\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    
    console_putstr("Kernel: Helios v0.1.0\n");
    console_putstr("Architecture: i386\n");
    
    pcb_t* current = get_current_process();
    if (current) {
        console_putstr("Current process: ");
        console_putstr(current->name);
        console_putstr(" (PID=");
        console_putdec(current->pid);
        console_putstr(")\n");
    }
    
    console_putstr("System uptime: ");
    console_putdec(get_time_ms() / 1000);
    console_putstr(" seconds\n");
    
    return 0;
}

int cmd_ls(int argc, char* argv[]) {
    const char* path = (argc > 1) ? argv[1] : "/";
    
    console_putstr("Directory ");
    console_putstr(path);
    console_putstr(":\n");
    console_putstr("  .\n");
    console_putstr("  ..\n");
    console_putstr("  dev/\n");
    console_putstr("  tmp/\n");
    console_putstr("  bin/\n");
    
    return 0;
}

int cmd_cat(int argc, char* argv[]) {
    if (argc < 2) {
        console_putstr("Usage: cat <file>\n");
        return -1;
    }
    
    console_putstr("Reading file: ");
    console_putstr(argv[1]);
    console_putstr("\n");
    console_putstr("(File system not fully implemented)\n");
    
    return 0;
}

int cmd_test(int argc, char* argv[]) {
    console_putstr("\nRunning system tests...\n");
    
    // 测试内存分配
    console_putstr("Testing kmalloc... ");
    void* ptr1 = kmalloc(128);
    void* ptr2 = kmalloc(256);
    void* ptr3 = kmalloc(512);
    
    if (ptr1 && ptr2 && ptr3) {
        console_putstr("OK\n");
    } else {
        console_putstr("FAILED\n");
    }
    
    console_putstr("Testing kfree... ");
    kfree(ptr1);
    kfree(ptr2);
    kfree(ptr3);
    console_putstr("OK\n");
    
    // 测试进程创建
    console_putstr("Testing process creation... ");
    pcb_t* proc = create_process("test_proc", NULL);
    if (proc) {
        console_putstr("OK (PID=");
        console_putdec(proc->pid);
        console_putstr(")\n");
    } else {
        console_putstr("FAILED\n");
    }
    
    // 测试系统调用
    console_putstr("Testing syscalls... ");
    asm volatile("int $0x80" : : "a"(9));  // getpid
    console_putstr("OK\n");
    
    console_putstr("\nAll tests completed!\n");
    return 0;
}