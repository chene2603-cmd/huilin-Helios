#include "system.h"
#include "syscall.h"
#include "string.h"

#define MAX_INPUT 256
#define MAX_ARGS 16

// 内建命令
static int shell_cd(int argc, char* argv[]);
static int shell_exit(int argc, char* argv[]);
static int shell_help(int argc, char* argv[]);

// 内建命令表
typedef struct {
    const char* name;
    int (*func)(int argc, char* argv[]);
    const char* help;
} builtin_cmd_t;

static builtin_cmd_t builtins[] = {
    {"cd", shell_cd, "cd [dir] - Change directory"},
    {"exit", shell_exit, "exit - Exit shell"},
    {"help", shell_help, "help - Show this help"},
    {NULL, NULL, NULL}
};

// Shell主函数
void _start(void) {
    char input[MAX_INPUT];
    char cwd[256];
    
    sys_write(1, "Helios Shell v0.1\n", 19);
    sys_write(1, "Type 'help' for help\n\n", 22);
    
    while (1) {
        // 显示提示符
        sys_getcwd(cwd, sizeof(cwd));
        sys_write(1, cwd, strlen(cwd));
        sys_write(1, "> ", 2);
        
        // 读取输入
        int len = sys_read(0, input, MAX_INPUT - 1);
        if (len <= 0) {
            continue;
        }
        
        // 移除换行符
        if (input[len - 1] == '\n') {
            input[len - 1] = '\0';
            len--;
        }
        
        // 跳过开头的空格
        char* cmd = input;
        while (*cmd == ' ') {
            cmd++;
        }
        
        if (strlen(cmd) == 0) {
            continue;
        }
        
        // 解析参数
        char* argv[MAX_ARGS];
        int argc = 0;
        
        char* token = strtok(cmd, " ");
        while (token && argc < MAX_ARGS) {
            argv[argc++] = token;
            token = strtok(NULL, " ");
        }
        
        if (argc == 0) {
            continue;
        }
        
        // 检查是否为内建命令
        int builtin_found = 0;
        for (int i = 0; builtins[i].name; i++) {
            if (strcmp(argv[0], builtins[i].name) == 0) {
                builtins[i].func(argc, argv);
                builtin_found = 1;
                break;
            }
        }
        
        if (builtin_found) {
            continue;
        }
        
        // 外部命令
        int pid = sys_fork();
        if (pid == 0) {
            // 子进程
            sys_execve(argv[0], argv, NULL);
            
            // 如果execve失败
            sys_write(2, "Command not found: ", 19);
            sys_write(2, argv[0], strlen(argv[0]));
            sys_write(2, "\n", 1);
            sys_exit(1);
        } else if (pid > 0) {
            // 父进程：等待子进程
            int status;
            sys_waitpid(pid, &status, 0);
        } else {
            sys_write(2, "Failed to fork\n", 15);
        }
    }
}

// 改变目录
static int shell_cd(int argc, char* argv[]) {
    const char* path = (argc > 1) ? argv[1] : "/";
    
    if (sys_chdir(path) != 0) {
        sys_write(2, "cd: cannot change to ", 21);
        sys_write(2, path, strlen(path));
        sys_write(2, "\n", 1);
        return 1;
    }
    
    return 0;
}

// 退出Shell
static int shell_exit(int argc, char* argv[]) {
    sys_exit(0);
    return 0;  // 永远不会执行
}

// 显示帮助
static int shell_help(int argc, char* argv[]) {
    sys_write(1, "\nBuilt-in commands:\n", 20);
    sys_write(1, "-----------------\n", 18);
    
    for (int i = 0; builtins[i].name; i++) {
        sys_write(1, "  ", 2);
        sys_write(1, builtins[i].help, strlen(builtins[i].help));
        sys_write(1, "\n", 1);
    }
    
    sys_write(1, "\nExternal commands:\n", 19);
    sys_write(1, "  echo, ls, cat, mem, ps, time, sysinfo\n", 40);
    
    return 0;
}