# huilin-Helios

[中文](README.md) | [English](README_EN.md) (TODO)

**huilin-Helios** 是一个**从零开始编写**的、面向x86架构的教学型操作系统内核。本项目的目标是深入理解计算机从加电到运行用户程序的完整过程，并亲手实现一个简洁、可运行的内核。

> **名称由来**：`huilin` 意为“灰磷”，`Helios` 是希腊神话中的太阳神，寓意“从基础材料中构建光明”。

## 🎯 当前状态与功能

本项目仍在积极开发中，目前已实现的核心模块包括：

*   **引导程序 (`boot/`)**: 实现从实模式到保护模式的切换，为内核加载做好准备。
*   **物理内存管理 (`include/pmm.h`)**: 实现了**【？请补充，例如：基于位图/伙伴系统的物理页帧分配与回收算法】**。
*   **内核定时器 (`kernel/timer.c`)**: 基于PIT(可编程间隔定时器)或**【？请补充，例如：HPET/LAPIC】** 实现了时钟中断，是任务调度的基础。
*   **基础框架**: 包括链接脚本 (`linker.ld`, `user.ld`)、Makefile构建系统等。

**近期目标 (TODO)**:
1.  [ ] 实现虚拟内存管理 (VMM) 与分页机制。
2.  [ ] 实现简单的进程/线程模型与调度器。
3.  [ ] 实现系统调用接口。
4.  [ ] 实现基础的控制台输出与键盘输入驱动。

## 🗂️ 项目结构
huilin-Helios/

├── boot/                   # 系统引导

│   └── boot.asm           # 主引导汇编程序

├── kernel/                # 内核核心代码

│   └── timer.c            # 定时器中断处理

├── include/               # 内核头文件

│   └── pmm.h              # 物理内存管理头文件

├── user/                  # 用户态程序（未来）

│   └── test.c             # 用户测试程序

├── linker.ld              # 内核链接脚本

├── user.ld                # 用户程序链接脚本

├── Makefile               # 主构建文件

└── Makefile.user          # 用户程序构建文件
## 🚀 快速开始 (构建与运行)

### 环境准备
你需要一个类Unix开发环境（Linux/macOS/WSL2）并安装以下工具：
bash

1. 编译工具链

sudo apt-get install build-essential    # Debian/Ubuntu

或使用 yum/dnf 对应包

2. 交叉编译器 (用于编译x86目标代码)

例如：gcc-multilib, nasm (汇编器)

sudo apt-get install gcc-multilib nasm

3. 模拟器 (用于运行内核)

sudo apt-get install qemu-system-x86
### 编译内核
在项目根目录下，执行一条命令即可完成编译：
make all

这条命令会：
1.  用 `nasm` 汇编 `boot/boot.asm`。
2.  用 `gcc` 编译所有内核C源文件。
3.  链接所有目标文件，根据 `linker.ld` 生成最终的内核映像文件 `huilin_helios.bin` (或 `kernel.bin`，具体名称请参考你的Makefile)。

### 在QEMU中运行
使用以下命令在QEMU模拟器中启动你的内核：

这条命令会：
1.  用 `nasm` 汇编 `boot/boot.asm`。
2.  用 `gcc` 编译所有内核C源文件。
3.  链接所有目标文件，根据 `linker.ld` 生成最终的内核映像文件 `huilin_helios.bin` (或 `kernel.bin`，具体名称请参考你的Makefile)。

### 在QEMU中运行
使用以下命令在QEMU模拟器中启动你的内核：
bash

make run

或手动运行：

qemu-system-i386 -kernel huilin_helios.bin -serial stdio
如果运行成功，你应该能在QEMU窗口或控制台中看到内核的初始输出（例如，从实模式切换到保护模式成功的提示，或定时器中断的调试信息）。

## 📖 代码导读

如果你希望了解某个特定模块的实现：

*   **启动流程**: 阅读 `boot/boot.asm`。这里是从BIOS加载、进入保护模式、然后跳转到 `kernel_main` 的起点。
*   **内存管理**: 阅读 `include/pmm.h` 和对应的 `.c` 源文件。这里定义了页帧的分配与释放接口。
*   **定时器**: 阅读 `kernel/timer.c`，了解如何设置中断描述符表(IDT)的时钟中断入口，并处理周期性中断。

## 🤝 如何贡献

目前这是一个个人教学项目，旨在记录和分享操作系统开发的学习过程。如果你有任何建议、发现了Bug，或者想一起探讨实现细节，欢迎：
1.  **提交Issue**: 提出问题或建议。
2.  **Fork & PR**: 如果你有具体的代码改进，欢迎提交Pull Request。

## 📄 许可证

本项目采用 **MIT 许可证**。详情请见 [LICENSE](LICENSE) 文件。

## 🙏 致谢

感谢所有开源操作系统先驱（如Linux, Minix, xv6）的启迪，以及网络上众多优秀的教程和分享者。

---

*最后更新: 2026-05-06*
*内核版本: 0.1.0-prealpha*
