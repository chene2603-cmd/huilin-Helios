#ifndef FS_H
#define FS_H

#include "system.h"

// 文件系统常量
#define FS_MAGIC 0xDEADBEEF
#define MAX_FILENAME 255
#define MAX_PATH 1024
#define BLOCK_SIZE 512
#define MAX_BLOCKS_PER_FILE 256
#define INODES_PER_BLOCK 8
#define DIRECT_BLOCKS 12
#define MAX_FILES 1024

// 文件类型
#define FT_UNKNOWN 0
#define FT_REGULAR 1
#define FT_DIRECTORY 2
#define FT_CHARDEV 3
#define FT_BLOCKDEV 4
#define FT_FIFO 5
#define FT_SOCKET 6
#define FT_SYMLINK 7

// 文件权限
#define FM_READ 0x01
#define FM_WRITE 0x02
#define FM_EXEC 0x04
#define FM_APPEND 0x08

// 文件打开标志
#define O_RDONLY 0x0000
#define O_WRONLY 0x0001
#define O_RDWR 0x0002
#define O_CREAT 0x0100
#define O_TRUNC 0x0200
#define O_APPEND 0x0400
#define O_EXCL 0x0800

// 超级块结构
typedef struct {
    uint32_t magic;         // 文件系统魔数
    uint32_t block_size;    // 块大小
    uint32_t blocks_count;  // 总块数
    uint32_t free_blocks;   // 空闲块数
    uint32_t inodes_count;  // inode总数
    uint32_t free_inodes;   // 空闲inode数
    uint32_t first_data_block;  // 第一个数据块
    uint32_t inode_table_block; // inode表起始块
    uint32_t root_inode;    // 根目录inode
    char volume_name[32];   // 卷名
} superblock_t;

// inode结构
typedef struct {
    uint32_t mode;          // 文件类型和权限
    uint32_t uid;           // 用户ID
    uint32_t gid;           // 组ID
    uint32_t size;          // 文件大小
    uint32_t blocks;        // 占用块数
    uint32_t ctime;         // 创建时间
    uint32_t mtime;         // 修改时间
    uint32_t atime;         // 访问时间
    uint32_t links;         // 硬链接数
    uint32_t direct[DIRECT_BLOCKS];  // 直接块指针
    uint32_t indirect;      // 一级间接块
    uint32_t double_indirect; // 二级间接块
} inode_t;

// 目录项结构
typedef struct {
    uint32_t inode;         // inode编号
    uint16_t rec_len;       // 记录长度
    uint8_t name_len;       // 文件名长度
    uint8_t file_type;      // 文件类型
    char name[];            // 文件名（可变长度）
} dir_entry_t;

// 文件控制块
typedef struct file {
    uint32_t inode;         // inode编号
    uint32_t mode;          // 打开模式
    uint32_t pos;           // 文件位置
    uint32_t flags;         // 文件标志
    struct file_ops* ops;   // 文件操作
    void* private_data;     // 私有数据
} file_t;

// 文件操作结构
typedef struct file_ops {
    int (*read)(file_t* file, void* buf, uint32_t count);
    int (*write)(file_t* file, const void* buf, uint32_t count);
    int (*seek)(file_t* file, int offset, int whence);
    int (*ioctl)(file_t* file, int cmd, void* arg);
    int (*flush)(file_t* file);
    int (*release)(file_t* file);
} file_ops_t;

// 文件系统操作
typedef struct fs_ops {
    int (*mount)(const char* device, const char* mountpoint);
    int (*umount)(const char* mountpoint);
    int (*open)(file_t* file, const char* path, int flags);
    int (*create)(file_t* file, const char* path, int mode);
    int (*unlink)(const char* path);
    int (*mkdir)(const char* path, int mode);
    int (*rmdir)(const char* path);
    int (*rename)(const char* oldpath, const char* newpath);
    int (*stat)(const char* path, void* buf);
    int (*readdir)(file_t* file, dir_entry_t* dirent, uint32_t count);
} fs_ops_t;

// 已挂载文件系统
typedef struct mounted_fs {
    char device[32];        // 设备名
    char mountpoint[256];   // 挂载点
    superblock_t* sb;       // 超级块
    fs_ops_t* ops;          // 文件系统操作
    void* private_data;     // 私有数据
    struct mounted_fs* next;
} mounted_fs_t;

// 文件系统函数声明
void init_fs(void);
int mount(const char* device, const char* mountpoint, const char* fstype);
int umount(const char* mountpoint);
int open(const char* path, int flags, int mode);
int close(int fd);
int read(int fd, void* buf, uint32_t count);
int write(int fd, const void* buf, uint32_t count);
int seek(int fd, int offset, int whence);
int unlink(const char* path);
int mkdir(const char* path, int mode);
int rmdir(const char* path);
int rename(const char* oldpath, const char* newpath);
int stat(const char* path, void* buf);
int readdir(int fd, void* buf, uint32_t count);
void show_fs_info(void);

#endif