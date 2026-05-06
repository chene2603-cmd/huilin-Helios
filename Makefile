# =============================================
#  Helios Kernel Makefile
# =============================================

ASM      = nasm
CC       = gcc
LD       = ld

# 编译标志
CFLAGS   = -m32 -ffreestanding -nostdinc -fno-builtin -fno-stack-protector \
           -nostartfiles -nodefaultlibs -Wall -Wextra -c -I./include
ASMFLAGS = -f elf32
LDFLAGS  = -T linker.ld -m elf_i386 -nostdlib

# 目录
BOOT_DIR   = boot
KERNEL_DIR = kernel
BUILD_DIR  = build

# 引导文件
BOOT_ASM = $(BOOT_DIR)/boot.asm
BOOT_OBJ = $(BUILD_DIR)/boot.o

# 内核 C 文件（自动收集）
KERNEL_C = $(wildcard $(KERNEL_DIR)/*.c)
KERNEL_OBJ = $(patsubst $(KERNEL_DIR)/%.c, $(BUILD_DIR)/%.o, $(KERNEL_C))

# 内核汇编文件
KERNEL_ASM = $(wildcard $(KERNEL_DIR)/*.asm)
KERNEL_ASM_OBJ = $(patsubst $(KERNEL_DIR)/%.asm, $(BUILD_DIR)/%.o, $(KERNEL_ASM))

# 所有目标文件
OBJS = $(BOOT_OBJ) $(KERNEL_OBJ) $(KERNEL_ASM_OBJ)

# 输出文件
KERNEL_BIN = $(BUILD_DIR)/kernel.bin
ISO_FILE   = helios.iso

# 默认目标
all: $(BUILD_DIR) $(KERNEL_BIN)

# 创建构建目录
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 链接内核
$(KERNEL_BIN): $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $^

# 编译 boot.asm（必须是 ELF32）
$(BUILD_DIR)/boot.o: $(BOOT_ASM)
	$(ASM) -f elf32 $< -o $@

# 编译内核汇编文件
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.asm
	$(ASM) $(ASMFLAGS) $< -o $@

# 编译内核 C 文件
$(BUILD_DIR)/%.o: $(KERNEL_DIR)/%.c
	$(CC) $(CFLAGS) $< -o $@

# ISO 生成（可选）
iso: $(KERNEL_BIN)
	mkdir -p iso/boot/grub
	cp $(KERNEL_BIN) iso/boot/kernel.bin
	echo 'set timeout=0' > iso/boot/grub/grub.cfg
	echo 'set default=0' >> iso/boot/grub/grub.cfg
	echo 'menuentry "Helios" {' >> iso/boot/grub/grub.cfg
	echo '    multiboot /boot/kernel.bin' >> iso/boot/grub/grub.cfg
	echo '}' >> iso/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_FILE) iso 2>/dev/null
	rm -rf iso

# QEMU 运行
run: $(KERNEL_BIN)
	qemu-system-i386 -kernel $(KERNEL_BIN) -serial stdio

# 调试模式
debug: $(KERNEL_BIN)
	qemu-system-i386 -s -S -kernel $(KERNEL_BIN) &
	sleep 1
	gdb -ex "target remote localhost:1234" -ex "symbol-file $(KERNEL_BIN)"

# 清理
clean:
	rm -rf $(BUILD_DIR) $(ISO_FILE) iso

.PHONY: all iso run debug clean