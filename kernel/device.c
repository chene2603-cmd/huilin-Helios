#include "system.h"
#include "screen.h"
#include "device.h"
#include "kheap.h"
#include "string.h"

// 设备列表
static device_t* device_list = NULL;

// 空设备
static int null_read(void* device, void* buf, uint32_t count) {
    return 0;  // 总是返回EOF
}

static int null_write(void* device, const void* buf, uint32_t count) {
    return count;  // 总是成功
}

static int null_init(void* device) {
    return 0;
}

static device_ops_t null_ops = {
    .init = null_init,
    .read = null_read,
    .write = null_write,
    .seek = NULL,
    .ioctl = NULL,
    .poll = NULL,
    .close = NULL
};

// 零设备
static int zero_read(void* device, void* buf, uint32_t count) {
    memset(buf, 0, count);
    return count;
}

static int zero_write(void* device, const void* buf, uint32_t count) {
    return count;  // 数据被丢弃
}

static device_ops_t zero_ops = {
    .init = null_init,
    .read = zero_read,
    .write = zero_write,
    .seek = NULL,
    .ioctl = NULL,
    .poll = NULL,
    .close = NULL
};

// 控制台设备
static int console_read(void* device, void* buf, uint32_t count) {
    // 从键盘读取
    char* buffer = (char*)buf;
    uint32_t i = 0;
    
    while (i < count) {
        uint8_t key = inb(0x60);
        
        if (key < 0x80) {  // 键按下
            char c = 0;
            switch (key) {
                case 0x1C: c = '\n'; break;  // 回车
                case 0x0E:  // 退格
                    if (i > 0) {
                        i--;
                        buffer[i] = '\0';
                    }
                    break;
                case 0x39: c = ' '; break;
                default:
                    // 简单键码转换
                    if (key >= 0x10 && key <= 0x1C) {
                        c = "qwertyuiop"[key - 0x10];
                    } else if (key >= 0x1E && key <= 0x26) {
                        c = "asdfghjkl"[key - 0x1E];
                    } else if (key >= 0x2C && key <= 0x32) {
                        c = "zxcvbnm"[key - 0x2C];
                    } else if (key >= 0x02 && key <= 0x0B) {
                        c = "1234567890-="[key - 0x02];
                    }
            }
            
            if (c && i < count) {
                buffer[i++] = c;
                
                if (c == '\n') {
                    break;
                }
            }
        }
    }
    
    return i;
}

static int console_write(void* device, const void* buf, uint32_t count) {
    const char* buffer = (const char*)buf;
    
    for (uint32_t i = 0; i < count; i++) {
        screen_putchar(buffer[i]);
    }
    
    return count;
}

static device_ops_t console_ops = {
    .init = null_init,
    .read = console_read,
    .write = console_write,
    .seek = NULL,
    .ioctl = NULL,
    .poll = NULL,
    .close = NULL
};

// 初始化设备管理器
void init_devices(void) {
    screen_putstr("Initializing Device Manager... ");
    
    device_list = NULL;
    
    // 注册标准设备
    
    // 空设备
    device_t* null_dev = (device_t*)kmalloc(sizeof(device_t));
    if (null_dev) {
        strcpy(null_dev->name, DEV_NULL);
        null_dev->type = DEV_CHAR;
        null_dev->flags = DEV_READABLE | DEV_WRITABLE;
        null_dev->size = 0;
        null_dev->private_data = NULL;
        null_dev->ops = &null_ops;
        null_dev->next = NULL;
        
        register_device(null_dev);
    }
    
    // 零设备
    device_t* zero_dev = (device_t*)kmalloc(sizeof(device_t));
    if (zero_dev) {
        strcpy(zero_dev->name, DEV_ZERO);
        zero_dev->type = DEV_CHAR;
        zero_dev->flags = DEV_READABLE | DEV_WRITABLE;
        zero_dev->size = 0;
        zero_dev->private_data = NULL;
        zero_dev->ops = &zero_ops;
        zero_dev->next = NULL;
        
        register_device(zero_dev);
    }
    
    // 控制台设备
    device_t* console_dev = (device_t*)kmalloc(sizeof(device_t));
    if (console_dev) {
        strcpy(console_dev->name, DEV_CONSOLE);
        console_dev->type = DEV_CHAR;
        console_dev->flags = DEV_READABLE | DEV_WRITABLE;
        console_dev->size = 0;
        console_dev->private_data = NULL;
        console_dev->ops = &console_ops;
        console_dev->next = NULL;
        
        register_device(console_dev);
    }
    
    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("OK\n");
}

// 注册设备
int register_device(device_t* device) {
    if (!device) {
        return -1;
    }
    
    // 检查是否已注册
    device_t* dev = device_list;
    while (dev) {
        if (strcmp(dev->name, device->name) == 0) {
            screen_setcolor(COLOR_YELLOW, COLOR_BLACK);
            screen_putstr("Device ");
            screen_putstr(device->name);
            screen_putstr(" already registered\n");
            return -1;
        }
        dev = dev->next;
    }
    
    // 添加到链表头部
    device->next = device_list;
    device_list = device;
    
    // 初始化设备
    if (device->ops && device->ops->init) {
        device->ops->init(device->private_data);
    }
    
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr("Registered device: ");
    screen_putstr(device->name);
    screen_putstr("\n");
    
    return 0;
}

// 注销设备
int unregister_device(const char* name) {
    device_t* dev = device_list;
    device_t* prev = NULL;
    
    while (dev) {
        if (strcmp(dev->name, name) == 0) {
            if (prev) {
                prev->next = dev->next;
            } else {
                device_list = dev->next;
            }
            
            kfree(dev);
            
            screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
            screen_putstr("Unregistered device: ");
            screen_putstr(name);
            screen_putstr("\n");
            
            return 0;
        }
        
        prev = dev;
        dev = dev->next;
    }
    
    return -1;
}

// 获取设备
device_t* get_device(const char* name) {
    device_t* dev = device_list;
    
    while (dev) {
        if (strcmp(dev->name, name) == 0) {
            return dev;
        }
        dev = dev->next;
    }
    
    return NULL;
}

// 从设备读取
int device_read(const char* name, void* buf, uint32_t count) {
    device_t* dev = get_device(name);
    
    if (!dev || !dev->ops || !dev->ops->read) {
        return -1;
    }
    
    return dev->ops->read(dev->private_data, buf, count);
}

// 向设备写入
int device_write(const char* name, const void* buf, uint32_t count) {
    device_t* dev = get_device(name);
    
    if (!dev || !dev->ops || !dev->ops->write) {
        return -1;
    }
    
    return dev->ops->write(dev->private_data, buf, count);
}

// 移动设备指针
int device_seek(const char* name, uint32_t offset, int whence) {
    device_t* dev = get_device(name);
    
    if (!dev || !dev->ops || !dev->ops->seek) {
        return -1;
    }
    
    return dev->ops->seek(dev->private_data, offset, whence);
}

// 设备控制
int device_ioctl(const char* name, int cmd, void* arg) {
    device_t* dev = get_device(name);
    
    if (!dev || !dev->ops || !dev->ops->ioctl) {
        return -1;
    }
    
    return dev->ops->ioctl(dev->private_data, cmd, arg);
}

// 显示设备列表
void show_devices(void) {
    screen_setcolor(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_putstr("\nDevice List:\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    
    screen_putstr("Name       Type    Flags  Size\n");
    screen_putstr("---------- ------- ------ -----\n");
    
    device_t* dev = device_list;
    while (dev) {
        screen_putstr(dev->name);
        for (int i = strlen(dev->name); i < 12; i++) {
            screen_putstr(" ");
        }
        
        screen_putstr("  ");
        switch (dev->type) {
            case DEV_CHAR: screen_putstr("char   "); break;
            case DEV_BLOCK: screen_putstr("block  "); break;
            case DEV_NET: screen_putstr("net    "); break;
            default: screen_putstr("unknown"); break;
        }
        
        screen_putstr("  ");
        
        if (dev->flags & DEV_READABLE) screen_putstr("R");
        if (dev->flags & DEV_WRITABLE) screen_putstr("W");
        if (dev->flags & DEV_SEEKABLE) screen_putstr("S");
        if (dev->flags & DEV_NONBLOCK) screen_putstr("N");
        
        for (int i = 0; i < 6; i++) {
            screen_putstr(" ");
        }
        
        screen_putstr("  ");
        if (dev->size > 0) {
            if (dev->size >= 1024 * 1024) {
                screen_putdec(dev->size / 1024 / 1024);
                screen_putstr(" MB");
            } else if (dev->size >= 1024) {
                screen_putdec(dev->size / 1024);
                screen_putstr(" KB");
            } else {
                screen_putdec(dev->size);
                screen_putstr(" B");
            }
        } else {
            screen_putstr("-");
        }
        
        screen_putstr("\n");
        
        dev = dev->next;
    }
}