#include "system.h"
#include "screen.h"
#include "fs.h"
#include "kheap.h"
#include "string.h"
#include "device.h"
#include "ata.h"

// 全局变量
static mounted_fs_t* mounted_fs_list = NULL;
static file_t* open_files[MAX_FILES];
static uint32_t next_fd = 3;  // 0,1,2 被标准流占用

// 初始化文件系统
void init_fs(void) {
    screen_putstr("Initializing File System... ");
    
    // 清空打开文件表
    for (int i = 0; i < MAX_FILES; i++) {
        open_files[i] = NULL;
    }
    
    // 初始化标准流
    file_t* stdin_file = (file_t*)kmalloc(sizeof(file_t));
    file_t* stdout_file = (file_t*)kmalloc(sizeof(file_t));
    file_t* stderr_file = (file_t*)kmalloc(sizeof(file_t));
    
    memset(stdin_file, 0, sizeof(file_t));
    memset(stdout_file, 0, sizeof(file_t));
    memset(stderr_file, 0, sizeof(file_t));
    
    stdin_file->inode = 0;
    stdout_file->inode = 1;
    stderr_file->inode = 2;
    
    open_files[0] = stdin_file;
    open_files[1] = stdout_file;
    open_files[2] = stderr_file;
    
    // 创建根文件系统
    mounted_fs_t* root_fs = (mounted_fs_t*)kmalloc(sizeof(mounted_fs_t));
    if (!root_fs) {
        screen_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
        screen_putstr("Failed to allocate root filesystem!\n");
        return;
    }
    
    memset(root_fs, 0, sizeof(mounted_fs_t));
    strcpy(root_fs->device, "ram0");
    strcpy(root_fs->mountpoint, "/");
    root_fs->next = NULL;
    
    // 创建根目录超级块
    root_fs->sb = (superblock_t*)kmalloc(sizeof(superblock_t));
    if (!root_fs->sb) {
        kfree(root_fs);
        screen_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
        screen_putstr("Failed to allocate superblock!\n");
        return;
    }
    
    memset(root_fs->sb, 0, sizeof(superblock_t));
    root_fs->sb->magic = FS_MAGIC;
    root_fs->sb->block_size = BLOCK_SIZE;
    root_fs->sb->blocks_count = 1024;
    root_fs->sb->free_blocks = 1000;
    root_fs->sb->inodes_count = 128;
    root_fs->sb->free_inodes = 120;
    root_fs->sb->first_data_block = 1;
    root_fs->sb->inode_table_block = 2;
    root_fs->sb->root_inode = 1;
    strcpy(root_fs->sb->volume_name, "rootfs");
    
    mounted_fs_list = root_fs;
    
    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("OK\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr("  Root filesystem mounted at /\n");
}

// 挂载文件系统
int mount(const char* device, const char* mountpoint, const char* fstype) {
    // 检查挂载点是否已挂载
    mounted_fs_t* fs = mounted_fs_list;
    while (fs) {
        if (strcmp(fs->mountpoint, mountpoint) == 0) {
            screen_setcolor(COLOR_YELLOW, COLOR_BLACK);
            screen_putstr("Mountpoint ");
            screen_putstr(mountpoint);
            screen_putstr(" is already mounted\n");
            return -1;
        }
        fs = fs->next;
    }
    
    // 创建新的挂载项
    mounted_fs_t* new_fs = (mounted_fs_t*)kmalloc(sizeof(mounted_fs_t));
    if (!new_fs) {
        return -1;
    }
    
    memset(new_fs, 0, sizeof(mounted_fs_t));
    strncpy(new_fs->device, device, sizeof(new_fs->device) - 1);
    strncpy(new_fs->mountpoint, mountpoint, sizeof(new_fs->mountpoint) - 1);
    
    // 添加到链表
    new_fs->next = mounted_fs_list;
    mounted_fs_list = new_fs;
    
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr("Mounted ");
    screen_putstr(device);
    screen_putstr(" at ");
    screen_putstr(mountpoint);
    screen_putstr("\n");
    
    return 0;
}

// 卸载文件系统
int umount(const char* mountpoint) {
    mounted_fs_t* fs = mounted_fs_list;
    mounted_fs_t* prev = NULL;
    
    while (fs) {
        if (strcmp(fs->mountpoint, mountpoint) == 0) {
            // 从链表中移除
            if (prev) {
                prev->next = fs->next;
            } else {
                mounted_fs_list = fs->next;
            }
            
            // 释放资源
            if (fs->sb) {
                kfree(fs->sb);
            }
            kfree(fs);
            
            screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
            screen_putstr("Unmounted ");
            screen_putstr(mountpoint);
            screen_putstr("\n");
            
            return 0;
        }
        
        prev = fs;
        fs = fs->next;
    }
    
    screen_setcolor(COLOR_YELLOW, COLOR_BLACK);
    screen_putstr("Mountpoint ");
    screen_putstr(mountpoint);
    screen_putstr(" not found\n");
    
    return -1;
}

// 打开文件
int open(const char* path, int flags, int mode) {
    // 查找挂载点
    mounted_fs_t* fs = mounted_fs_list;
    while (fs) {
        if (strncmp(path, fs->mountpoint, strlen(fs->mountpoint)) == 0) {
            break;
        }
        fs = fs->next;
    }
    
    if (!fs) {
        // 使用根文件系统
        fs = mounted_fs_list;
    }
    
    // 分配文件控制块
    file_t* file = (file_t*)kmalloc(sizeof(file_t));
    if (!file) {
        return -1;
    }
    
    memset(file, 0, sizeof(file_t));
    file->mode = flags;
    file->pos = 0;
    file->flags = 0;
    
    // 特殊文件处理
    if (strcmp(path, "/dev/tty") == 0 || 
        strcmp(path, "/dev/console") == 0) {
        // 控制台设备
        file->inode = 0;
    } else if (strcmp(path, "/dev/stdout") == 0) {
        file->inode = 1;
    } else if (strcmp(path, "/dev/stderr") == 0) {
        file->inode = 2;
    } else {
        // 普通文件
        file->inode = 3;  // 第一个用户文件
    }
    
    // 查找空闲文件描述符
    int fd = -1;
    for (int i = 0; i < MAX_FILES; i++) {
        if (open_files[i] == NULL) {
            fd = i;
            open_files[i] = file;
            break;
        }
    }
    
    if (fd == -1) {
        kfree(file);
        return -1;
    }
    
    return fd;
}

// 关闭文件
int close(int fd) {
    if (fd < 0 || fd >= MAX_FILES || open_files[fd] == NULL) {
        return -1;
    }
    
    kfree(open_files[fd]);
    open_files[fd] = NULL;
    
    return 0;
}

// 读取文件
int read(int fd, void* buf, uint32_t count) {
    if (fd < 0 || fd >= MAX_FILES || open_files[fd] == NULL) {
        return -1;
    }
    
    file_t* file = open_files[fd];
    
    // 特殊文件处理
    if (file->inode == 0) {  // stdin
        return sys_read(fd, buf, count);
    }
    
    // TODO: 普通文件读取
    return 0;
}

// 写入文件
int write(int fd, const void* buf, uint32_t count) {
    if (fd < 0 || fd >= MAX_FILES || open_files[fd] == NULL) {
        return -1;
    }
    
    file_t* file = open_files[fd];
    
    // 特殊文件处理
    if (file->inode == 1 || file->inode == 2) {  // stdout or stderr
        return sys_write(fd, buf, count);
    }
    
    // TODO: 普通文件写入
    return 0;
}

// 移动文件指针
int seek(int fd, int offset, int whence) {
    if (fd < 0 || fd >= MAX_FILES || open_files[fd] == NULL) {
        return -1;
    }
    
    file_t* file = open_files[fd];
    
    switch (whence) {
        case 0:  // SEEK_SET
            file->pos = offset;
            break;
        case 1:  // SEEK_CUR
            file->pos += offset;
            break;
        case 2:  // SEEK_END
            // TODO: 需要文件大小
            break;
        default:
            return -1;
    }
    
    return file->pos;
}

// 删除文件
int unlink(const char* path) {
    // TODO: 实现文件删除
    return 0;
}

// 创建目录
int mkdir(const char* path, int mode) {
    // TODO: 实现目录创建
    return 0;
}

// 删除目录
int rmdir(const char* path) {
    // TODO: 实现目录删除
    return 0;
}

// 重命名文件
int rename(const char* oldpath, const char* newpath) {
    // TODO: 实现重命名
    return 0;
}

// 获取文件状态
int stat(const char* path, void* buf) {
    // TODO: 实现stat
    return 0;
}

// 读取目录
int readdir(int fd, void* buf, uint32_t count) {
    // TODO: 实现readdir
    return 0;
}

// 显示文件系统信息
void show_fs_info(void) {
    screen_setcolor(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_putstr("\nFile System Information:\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    
    mounted_fs_t* fs = mounted_fs_list;
    
    screen_putstr("Device    Mountpoint  Type    Size\n");
    screen_putstr("--------  ----------  ------  -----\n");
    
    while (fs) {
        screen_putstr(fs->device);
        for (int i = strlen(fs->device); i < 10; i++) {
            screen_putstr(" ");
        }
        
        screen_putstr("  ");
        screen_putstr(fs->mountpoint);
        for (int i = strlen(fs->mountpoint); i < 12; i++) {
            screen_putstr(" ");
        }
        
        screen_putstr("  ");
        
        if (fs->sb) {
            screen_putstr("heliofs  ");
            screen_putdec(fs->sb->blocks_count * BLOCK_SIZE / 1024);
            screen_putstr(" KB");
        } else {
            screen_putstr("unknown  -");
        }
        
        screen_putstr("\n");
        
        fs = fs->next;
    }
    
    // 显示打开文件
    screen_putstr("\nOpen Files:\n");
    for (int i = 0; i < MAX_FILES; i++) {
        if (open_files[i]) {
            screen_putstr("  fd ");
            screen_putdec(i);
            screen_putstr(": inode=");
            screen_putdec(open_files[i]->inode);
            screen_putstr(", pos=");
            screen_putdec(open_files[i]->pos);
            screen_putstr("\n");
        }
    }
}