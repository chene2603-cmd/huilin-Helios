#ifndef DEVICE_H
#define DEVICE_H

#include "system.h"

// 设备类型
#define DEV_UNKNOWN 0
#define DEV_CHAR 1    // 字符设备
#define DEV_BLOCK 2   // 块设备
#define DEV_NET 3     // 网络设备

// 设备标志
#define DEV_READABLE 0x01
#define DEV_WRITABLE 0x02
#define DEV_SEEKABLE 0x04
#define DEV_NONBLOCK 0x08

// 设备操作结构
typedef struct device_ops {
    int (*init)(void* device);
    int (*read)(void* device, void* buf, uint32_t count);
    int (*write)(void* device, const void* buf, uint32_t count);
    int (*seek)(void* device, uint32_t offset, int whence);
    int (*ioctl)(void* device, int cmd, void* arg);
    int (*poll)(void* device, int events);
    int (*close)(void* device);
} device_ops_t;

// 设备结构
typedef struct device {
    char name[32];          // 设备名
    int type;               // 设备类型
    int flags;              // 设备标志
    uint32_t size;          // 设备大小
    void* private_data;     // 私有数据
    device_ops_t* ops;      // 设备操作
    struct device* next;    // 链表指针
} device_t;

// 设备管理函数
void init_devices(void);
int register_device(device_t* device);
int unregister_device(const char* name);
device_t* get_device(const char* name);
int device_read(const char* name, void* buf, uint32_t count);
int device_write(const char* name, const void* buf, uint32_t count);
int device_seek(const char* name, uint32_t offset, int whence);
int device_ioctl(const char* name, int cmd, void* arg);
void show_devices(void);

// 标准设备
#define DEV_CONSOLE "console"
#define DEV_TTY "tty"
#define DEV_NULL "null"
#define DEV_ZERO "zero"
#define DEV_RANDOM "random"
#define DEV_URANDOM "urandom"
#define DEV_MEM "mem"
#define DEV_PORT "port"

#endif