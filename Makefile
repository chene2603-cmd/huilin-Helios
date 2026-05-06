# Helios OS Makefile
# 编译工具定义
AS = nasm
CC = gcc
LD = ld
QEMU = qemu-system-i386
GRUB_MKRESCUE = grub-mkrescue

# 编译选项
ASFLAGS = -f elf32
CFLAGS = -m32 -ffreestanding -fno-pic -fno-builtin -nostdlib \
         -Wall -Wextra -Werror -std=c99 -Iinclude
LDFLAGS = -m elf_i386 -T linker.ld -nostdlib

# 目录定义
SRC_DIR = kernel
BOOT_DIR = boot
BUILD_DIR = build
ISO_DIR = iso
BOOT_DIR = $(ISO_DIR)/boot
GRUB_DIR = $(ISO_DIR)/boot/grub

# 源文件
KERNEL_SRCS = $(wildcard $(SRC_DIR)/*.c)
KERNEL_ASM_SRCS = $(wildcard $(SRC_DIR)/*.asm)
BOOT_SRCS = $(wildcard $(BOOT_DIR)/*.asm)

# 目标文件
KERNEL_OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(KERNEL_SRCS)) \
              $(patsubst $(SRC_DIR)/%.asm,$(BUILD_DIR)/%.o,$(KERNEL_ASM_SRCS))
BOOT_OBJS = $(patsubst $(BOOT_DIR)/%.asm,$(BUILD_DIR)/%.o,$(BOOT_SRCS))

# 最终目标
TARGET = helios.bin
ISO = helios.iso

# 默认目标
all: $(BUILD_DIR) $(ISO)

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 编译内核C文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# 编译汇编文件
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.asm
	$(AS) $(ASFLAGS) $< -o $@

# 编译引导文件
$(BUILD_DIR)/boot.o: $(BOOT_DIR)/boot.asm
	$(AS) -f bin $< -o $@

# 链接内核
$(BUILD_DIR)/kernel.bin: $(KERNEL_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# 创建ISO
$(ISO): $(BUILD_DIR)/kernel.bin
	# 创建ISO目录结构
	mkdir -p $(GRUB_DIR)
	
	# 复制内核
	cp $(BUILD_DIR)/kernel.bin $(BOOT_DIR)/
	
	# 创建GRUB配置
	echo 'set timeout=0' > $(GRUB_DIR)/grub.cfg
	echo 'set default=0' >> $(GRUB_DIR)/grub.cfg
	echo '' >> $(GRUB_DIR)/grub.cfg
	echo 'menuentry "Helios OS" {' >> $(GRUB_DIR)/grub.cfg
	echo '  multiboot /boot/kernel.bin' >> $(GRUB_DIR)/grub.cfg
	echo '  boot' >> $(GRUB_DIR)/grub.cfg
	echo '}' >> $(GRUB_DIR)/grub.cfg
	
	# 创建ISO
	$(GRUB_MKRESCUE) -o $(ISO) $(ISO_DIR)
	
	# 清理临时目录
	rm -rf $(ISO_DIR)

# 运行QEMU
run: $(ISO)
	$(QEMU) -cdrom $(ISO) -monitor stdio

# 调试运行
debug: $(ISO)
	$(QEMU) -s -S -cdrom $(ISO) -monitor stdio &
	gdb -ex "target remote localhost:1234" \
	    -ex "symbol-file $(BUILD_DIR)/kernel.bin" \
	    -ex "break kernel_main" \
	    -ex "continue"

# 清理
clean:
	rm -rf $(BUILD_DIR) $(ISO_DIR) $(ISO) *.iso

# 格式化代码
format:
	find . -name "*.c" -o -name "*.h" | xargs clang-format -i
	find . -name "*.asm" | xargs nasmfmt -i

.PHONY: all run debug clean format