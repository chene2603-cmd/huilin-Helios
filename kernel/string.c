#include "system.h"

// 计算字符串长度
uint32_t strlen(const char* str) {
    uint32_t len = 0;
    while (str[len]) len++;
    return len;
}

// 复制字符串
char* strcpy(char* dest, const char* src) {
    char* d = dest;
    while ((*d++ = *src++));
    return dest;
}

// 复制指定长度的字符串
char* strncpy(char* dest, const char* src, uint32_t n) {
    char* d = dest;
    uint32_t i = 0;
    
    while (i < n && src[i]) {
        d[i] = src[i];
        i++;
    }
    
    while (i < n) {
        d[i] = '\0';
        i++;
    }
    
    return dest;
}

// 比较字符串
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// 比较指定长度的字符串
int strncmp(const char* s1, const char* s2, uint32_t n) {
    for (uint32_t i = 0; i < n; i++) {
        if (s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
        if (s1[i] == '\0') {
            return 0;
        }
    }
    return 0;
}

// 查找字符
char* strchr(const char* s, int c) {
    while (*s != '\0') {
        if (*s == (char)c) {
            return (char*)s;
        }
        s++;
    }
    return NULL;
}

// 查找子串
char* strstr(const char* haystack, const char* needle) {
    if (!*needle) return (char*)haystack;
    
    for (uint32_t i = 0; haystack[i]; i++) {
        if (haystack[i] != needle[0]) continue;
        
        uint32_t j = 0;
        while (needle[j] && haystack[i + j] == needle[j]) {
            j++;
        }
        
        if (!needle[j]) {
            return (char*)(haystack + i);
        }
    }
    
    return NULL;
}

// 内存比较
int memcmp(const void* s1, const void* s2, uint32_t n) {
    const uint8_t* p1 = s1;
    const uint8_t* p2 = s2;
    
    for (uint32_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) {
            return p1[i] - p2[i];
        }
    }
    
    return 0;
}

// 内存设置
void* memset(void* dest, int val, uint32_t len) {
    uint8_t* d = dest;
    for (uint32_t i = 0; i < len; i++) {
        d[i] = (uint8_t)val;
    }
    return dest;
}

// 内存复制
void* memcpy(void* dest, const void* src, uint32_t len) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    
    for (uint32_t i = 0; i < len; i++) {
        d[i] = s[i];
    }
    
    return dest;
}

// 内存移动
void* memmove(void* dest, const void* src, uint32_t len) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    
    if (d < s) {
        for (uint32_t i = 0; i < len; i++) {
            d[i] = s[i];
        }
    } else {
        for (uint32_t i = len; i > 0; i--) {
            d[i-1] = s[i-1];
        }
    }
    
    return dest;
}