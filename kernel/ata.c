#include "system.h"
#include "screen.h"
#include "ata.h"
#include "kheap.h"
#include "string.h"
#include "isr.h"

// 全局变量
static ata_channel_t channels[2];
static ata_device_t devices[4];
static uint8_t ata_initialized = 0;

// 等待BSY位清除
static void ata_wait_busy(uint16_t port) {
    while (inb(port + ATA_STATUS) & ATA_SR_BSY) {
        asm volatile("pause");
    }
}

// 等待DRQ位置位
static void ata_wait_drq(uint16_t port) {
    while (!(inb(port + ATA_STATUS) & ATA_SR_DRQ)) {
        asm volatile("pause");
    }
}

// 等待设备就绪
static int ata_wait_ready(uint16_t port, int advanced) {
    uint8_t status;
    
    for (int i = 0; i < 4; i++) {
        inb(port + ATA_ALT_STATUS);
    }
    
    ata_wait_busy(port);
    
    if (advanced) {
        status = inb(port + ATA_STATUS);
        
        if (status & ATA_SR_ERR) {
            screen_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
            screen_putstr("ATA device error\n");
            return 0;
        }
        
        if (status & ATA_SR_DF) {
            screen_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
            screen_putstr("ATA device fault\n");
            return 0;
        }
        
        if (!(status & ATA_SR_DRDY)) {
            screen_setcolor(COLOR_YELLOW, COLOR_BLACK);
            screen_putstr("ATA device not ready\n");
            return 0;
        }
    }
    
    return 1;
}

// 选择驱动器
static void ata_select_drive(uint8_t drive) {
    uint8_t channel = drive / 2;
    uint8_t slave = drive % 2;
    
    outb(channels[channel].base + ATA_DRIVE_HEAD, 0xA0 | (slave << 4));
    
    for (int i = 0; i < 4; i++) {
        inb(channels[channel].base + ATA_ALT_STATUS);
    }
}

// 探测ATA设备
static int ata_probe(uint8_t drive) {
    uint8_t channel = drive / 2;
    uint8_t slave = drive % 2;
    
    // 选择驱动器
    ata_select_drive(drive);
    
    // 发送IDENTIFY命令
    outb(channels[channel].base + ATA_COMMAND, ATA_CMD_IDENTIFY);
    
    // 等待响应
    uint8_t status = inb(channels[channel].base + ATA_STATUS);
    if (status == 0) {
        // 设备不存在
        return 0;
    }
    
    // 等待BSY清除
    ata_wait_busy(channels[channel].base);
    
    status = inb(channels[channel].base + ATA_STATUS);
    
    // 检查LBA mid和high
    uint8_t lba_mid = inb(channels[channel].base + ATA_SECTOR_NUM);
    uint8_t lba_high = inb(channels[channel].base + ATA_CYL_LOW);
    
    if (lba_mid || lba_high) {
        // ATAPI设备
        return 0;
    }
    
    // 等待设备就绪
    if (!ata_wait_ready(channels[channel].base, 1)) {
        return 0;
    }
    
    // 读取IDENTIFY数据
    ata_identify_t* identify = &devices[drive].identify;
    
    for (int i = 0; i < 256; i++) {
        ((uint16_t*)identify)[i] = inw(channels[channel].base);
    }
    
    // 处理字符串
    char* str = (char*)identify;
    
    // 序列号
    for (int i = 0; i < 20; i += 2) {
        char c1 = str[54 + i];
        char c2 = str[54 + i + 1];
        str[54 + i] = c2;
        str[54 + i + 1] = c1;
    }
    str[74] = '\0';
    
    // 固件版本
    for (int i = 0; i < 8; i += 2) {
        char c1 = str[46 + i];
        char c2 = str[46 + i + 1];
        str[46 + i] = c2;
        str[46 + i + 1] = c1;
    }
    str[54] = '\0';
    
    // 型号
    for (int i = 0; i < 40; i += 2) {
        char c1 = str[20 + i];
        char c2 = str[20 + i + 1];
        str[20 + i] = c2;
        str[20 + i + 1] = c1;
    }
    str[60] = '\0';
    
    // 填充设备信息
    devices[drive].present = 1;
    devices[drive].type = ATA_ATA;
    devices[drive].channel = channel;
    devices[drive].drive = slave;
    
    strncpy(devices[drive].model, identify->model, 40);
    devices[drive].model[40] = '\0';
    
    // 计算大小
    if (identify->total_sectors) {
        devices[drive].size = identify->total_sectors;
    } else {
        devices[drive].size = identify->cylinders * identify->heads * identify->sectors;
    }
    
    return 1;
}

// 初始化ATA
void init_ata(void) {
    screen_putstr("Initializing ATA Controller... ");
    
    // 初始化通道
    channels[0].base = 0x1F0;
    channels[0].ctrl = 0x3F6;
    channels[0].bmide = 0;
    channels[0].nIEN = 0;
    
    channels[1].base = 0x170;
    channels[1].ctrl = 0x376;
    channels[1].bmide = 0;
    channels[1].nIEN = 0;
    
    // 初始化设备
    for (int i = 0; i < 4; i++) {
        devices[i].present = 0;
        devices[i].size = 0;
        devices[i].model[0] = '\0';
    }
    
    // 探测设备
    int found = 0;
    for (int i = 0; i < 4; i++) {
        if (ata_probe(i)) {
            found++;
        }
    }
    
    ata_initialized = 1;
    
    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("OK\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr("  Found ");
    screen_putdec(found);
    screen_putstr(" ATA device(s)\n");
}

// 读取扇区
int ata_read_sectors(uint8_t drive, uint32_t lba, uint8_t num, void* buf) {
    if (!ata_initialized || !devices[drive].present) {
        return 0;
    }
    
    if (num == 0) {
        return 0;
    }
    
    uint8_t channel = devices[drive].channel;
    uint8_t slave = devices[drive].drive;
    
    // 选择驱动器
    ata_select_drive(drive);
    
    // 等待设备就绪
    if (!ata_wait_ready(channels[channel].base, 1)) {
        return 0;
    }
    
    // 设置LBA地址
    outb(channels[channel].base + ATA_SECTOR_COUNT, num);
    outb(channels[channel].base + ATA_SECTOR_NUM, (uint8_t)lba);
    outb(channels[channel].base + ATA_CYL_LOW, (uint8_t)(lba >> 8));
    outb(channels[channel].base + ATA_CYL_HIGH, (uint8_t)(lba >> 16));
    outb(channels[channel].base + ATA_DRIVE_HEAD, 0xE0 | (slave << 4) | ((lba >> 24) & 0x0F));
    
    // 发送读命令
    outb(channels[channel].base + ATA_COMMAND, ATA_CMD_READ_PIO);
    
    uint16_t* buffer = (uint16_t*)buf;
    
    for (int s = 0; s < num; s++) {
        // 等待设备就绪
        if (!ata_wait_ready(channels[channel].base, 1)) {
            return s;
        }
        
        // 读取数据
        for (int i = 0; i < 256; i++) {
            buffer[i] = inw(channels[channel].base);
        }
        
        buffer += 256;
    }
    
    return num;
}

// 写入扇区
int ata_write_sectors(uint8_t drive, uint32_t lba, uint8_t num, const void* buf) {
    if (!ata_initialized || !devices[drive].present) {
        return 0;
    }
    
    if (num == 0) {
        return 0;
    }
    
    uint8_t channel = devices[drive].channel;
    uint8_t slave = devices[drive].drive;
    
    // 选择驱动器
    ata_select_drive(drive);
    
    // 等待设备就绪
    if (!ata_wait_ready(channels[channel].base, 1)) {
        return 0;
    }
    
    // 设置LBA地址
    outb(channels[channel].base + ATA_SECTOR_COUNT, num);
    outb(channels[channel].base + ATA_SECTOR_NUM, (uint8_t)lba);
    outb(channels[channel].base + ATA_CYL_LOW, (uint8_t)(lba >> 8));
    outb(channels[channel].base + ATA_CYL_HIGH, (uint8_t)(lba >> 16));
    outb(channels[channel].base + ATA_DRIVE_HEAD, 0xE0 | (slave << 4) | ((lba >> 24) & 0x0F));
    
    // 发送写命令
    outb(channels[channel].base + ATA_COMMAND, ATA_CMD_WRITE_PIO);
    
    const uint16_t* buffer = (const uint16_t*)buf;
    
    for (int s = 0; s < num; s++) {
        // 等待设备就绪
        if (!ata_wait_ready(channels[channel].base, 1)) {
            return s;
        }
        
        // 写入数据
        for (int i = 0; i < 256; i++) {
            outw(channels[channel].base, buffer[i]);
        }
        
        buffer += 256;
        
        // 发送缓存刷新
        outb(channels[channel].base + ATA_COMMAND, ATA_CMD_CACHE_FLUSH);
        
        // 等待刷新完成
        ata_wait_busy(channels[channel].base);
    }
    
    return num;
}

// 获取设备信息
int ata_identify(uint8_t drive, ata_identify_t* identify) {
    if (!ata_initialized || !devices[drive].present || !identify) {
        return 0;
    }
    
    memcpy(identify, &devices[drive].identify, sizeof(ata_identify_t));
    return 1;
}

// 获取设备大小
uint32_t get_ata_size(uint8_t drive) {
    if (!ata_initialized || !devices[drive].present) {
        return 0;
    }
    
    return devices[drive].size;
}

// 显示ATA信息
void show_ata_info(void) {
    screen_setcolor(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_putstr("\nATA Devices:\n");
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    
    for (int i = 0; i < 4; i++) {
        if (devices[i].present) {
            screen_putstr("  Device ");
            screen_putdec(i);
            screen_putstr(": ");
            screen_putstr(devices[i].model);
            screen_putstr(" (");
            
            uint32_t size_mb = devices[i].size * 512 / 1024 / 1024;
            if (size_mb >= 1024) {
                screen_putdec(size_mb / 1024);
                screen_putstr(" GB");
            } else {
                screen_putdec(size_mb);
                screen_putstr(" MB");
            }
            
            screen_putstr(")\n");
        }
    }
}