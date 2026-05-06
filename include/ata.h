#ifndef ATA_H
#define ATA_H

#include "system.h"

// ATA端口定义
#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_FEATURES    0x1F1
#define ATA_SECTOR_COUNT 0x1F2
#define ATA_SECTOR_NUM  0x1F3
#define ATA_CYL_LOW     0x1F4
#define ATA_CYL_HIGH    0x1F5
#define ATA_DRIVE_HEAD  0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7
#define ATA_ALT_STATUS  0x3F6
#define ATA_DRIVE_ADDR  0x3F6

// ATA状态位
#define ATA_SR_BSY      0x80
#define ATA_SR_DRDY     0x40
#define ATA_SR_DF       0x20
#define ATA_SR_DSC      0x10
#define ATA_SR_DRQ      0x08
#define ATA_SR_CORR     0x04
#define ATA_SR_IDX      0x02
#define ATA_SR_ERR      0x01

// ATA错误位
#define ATA_ER_BBK      0x80
#define ATA_ER_UNC      0x40
#define ATA_ER_MC       0x20
#define ATA_ER_IDNF     0x10
#define ATA_ER_MCR      0x08
#define ATA_ER_ABRT     0x04
#define ATA_ER_TK0NF    0x02
#define ATA_ER_AMNF     0x01

// ATA命令
#define ATA_CMD_READ_PIO        0x20
#define ATA_CMD_READ_PIO_EXT    0x24
#define ATA_CMD_READ_DMA        0xC8
#define ATA_CMD_READ_DMA_EXT    0x25
#define ATA_CMD_WRITE_PIO       0x30
#define ATA_CMD_WRITE_PIO_EXT   0x34
#define ATA_CMD_WRITE_DMA       0xCA
#define ATA_CMD_WRITE_DMA_EXT   0x35
#define ATA_CMD_CACHE_FLUSH     0xE7
#define ATA_CMD_CACHE_FLUSH_EXT 0xEA
#define ATA_CMD_PACKET          0xA0
#define ATA_CMD_IDENTIFY_PACKET 0xA1
#define ATA_CMD_IDENTIFY        0xEC

// ATA设备类型
#define ATA_ATA      0x00
#define ATA_ATAPI    0x01
#define ATA_SATA     0x02
#define ATA_PATAPI   0x03
#define ATA_SATAPI   0x04
#define ATA_UNKNOWN  0x05

// ATA设备信息结构
typedef struct {
    uint16_t config;          // 配置信息
    uint16_t cylinders;       // 柱面数
    uint16_t reserved1;
    uint16_t heads;           // 磁头数
    uint16_t reserved2[2];
    uint16_t sectors;         // 每磁道扇区数
    uint16_t reserved3[3];
    char serial[20];          // 序列号
    uint16_t reserved4[2];
    char firmware[8];         // 固件版本
    char model[40];           // 型号
    uint16_t sectors_per_int; // 每中断扇区数
    uint16_t capabilities;    // 能力
    uint16_t reserved5[3];
    uint32_t total_sectors;   // 总扇区数
    uint16_t reserved6[194];
} __attribute__((packed)) ata_identify_t;

// ATA设备结构
typedef struct {
    uint8_t present;          // 设备是否存在
    uint8_t type;             // 设备类型
    uint8_t channel;          // 通道号
    uint8_t drive;            // 驱动器号
    uint16_t signature;       // 设备签名
    uint16_t capabilities;    // 能力
    uint32_t command_sets;    // 命令集
    uint32_t size;            // 大小（扇区数）
    char model[41];           // 型号
    ata_identify_t identify;  // IDENTIFY数据
} ata_device_t;

// ATA通道结构
typedef struct {
    uint16_t base;            // 基础端口
    uint16_t ctrl;            // 控制端口
    uint16_t bmide;           // 总线主IDE端口
    uint8_t nIEN;             // 中断使能
} ata_channel_t;

// 函数声明
void init_ata(void);
int ata_read_sectors(uint8_t drive, uint32_t lba, uint8_t num, void* buf);
int ata_write_sectors(uint8_t drive, uint32_t lba, uint8_t num, const void* buf);
int ata_identify(uint8_t drive, ata_identify_t* identify);
void show_ata_info(void);
uint32_t get_ata_size(uint8_t drive);

#endif