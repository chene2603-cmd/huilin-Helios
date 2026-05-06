#ifndef SYSTEM_H
#define SYSTEM_H

// 基本数据类型定义
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

typedef signed char int8_t;
typedef signed short int16_t;
typedef signed int int32_t;
typedef signed long long int64_t;

// 标准类型定义
#define NULL ((void*)0)
#define bool int
#define true 1
#define false 0

// 端口输入输出
static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void io_wait(void) {
    outb(0x80, 0);
}

// 内存操作
#define memset(dest, val, len) do { \
    uint8_t *d = (uint8_t*)dest; \
    for (uint32_t i = 0; i < len; i++) \
        d[i] = (uint8_t)val; \
} while(0)

#define memcpy(dest, src, len) do { \
    uint8_t *s = (uint8_t*)src, *d = (uint8_t*)dest; \
    for (uint32_t i = 0; i < len; i++) \
        d[i] = s[i]; \
} while(0)

// 中断控制
#define CLI asm volatile("cli")
#define STI asm volatile("sti")
#define HLT asm volatile("hlt")

// 临界区保护
typedef struct {
    uint32_t eflags;
} spinlock_t;

#define spin_lock(lock) do { \
    uint32_t flags; \
    asm volatile("pushf\n\tpopl %0" : "=r"(flags)); \
    CLI; \
    (lock)->eflags = flags; \
} while(0)

#define spin_unlock(lock) do { \
    uint32_t flags = (lock)->eflags; \
    asm volatile("pushl %0\n\tpopf" : : "r"(flags)); \
} while(0)

#endif