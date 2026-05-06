#ifndef CONSOLE_H
#define CONSOLE_H

#include "system.h"

// 控制台缓冲区大小
#define CONSOLE_BUFFER_SIZE 4096
#define HISTORY_SIZE 10
#define MAX_ARGS 16

// 命令结构
typedef struct {
    const char* name;
    const char* description;
    int (*handler)(int argc, char* argv[]);
} command_t;

// 控制台结构
typedef struct {
    char buffer[CONSOLE_BUFFER_SIZE];
    uint32_t pos;
    uint32_t start;
    uint32_t end;
    
    char history[HISTORY_SIZE][256];
    int history_pos;
    int history_count;
    
    int escape_state;
    char escape_buffer[16];
    int escape_pos;
    
    int cursor_x;
    int cursor_y;
} console_t;

// 控制台函数
void init_console(void);
void console_putchar(char c);
void console_putstr(const char* str);
void console_puthex(uint32_t n);
void console_putdec(uint32_t n);
void console_clear(void);
void console_process(void);
void console_execute(const char* cmd);
void console_prompt(void);
int console_readline(char* buf, uint32_t size);

// 命令处理
int register_command(const char* name, const char* desc, int (*handler)(int, char**));
int execute_command(const char* cmd);
void show_help(int argc, char* argv[]);

// 标准命令
int cmd_help(int argc, char* argv[]);
int cmd_clear(int argc, char* argv[]);
int cmd_echo(int argc, char* argv[]);
int cmd_mem(int argc, char* argv[]);
int cmd_ps(int argc, char* argv[]);
int cmd_time(int argc, char* argv[]);
int cmd_sysinfo(int argc, char* argv[]);
int cmd_ls(int argc, char* argv[]);
int cmd_cat(int argc, char* argv[]);
int cmd_test(int argc, char* argv[]);

#endif