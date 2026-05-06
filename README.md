# huilin-Helios

**huilin-Helios** 是一个**从零开始编写**的、面向x86架构的教学型操作系统内核。本项目的目标是深入理解计算机从加电到运行用户程序的完整过程，并亲手实现一个简洁、可运行的内核。

> **名称由来**：`huilin` 意为“灰磷”，`Helios` 是希腊神话中的太阳神，寓意“从基础材料中构建光明”。

## 🎯 当前状态与功能

本项目仍在积极开发中，目前已实现的核心模块包括：

*   **引导程序 (`boot/`)**: 实现从实模式到保护模式的切换，为内核加载做好准备。
*   **物理内存管理 (`include/pmm.h`)**: 实现了基于物理内存页帧的管理框架。
*   **内核定时器 (`kernel/timer.c`)**: 基于PIT(可编程间隔定时器)的时钟中断框架。
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

│   ├── main.c            # 内核主入口

│   └── timer.c           # 定时器中断处理

├── include/               # 内核头文件

│   ├── pmm.h             # 物理内存管理

│   └── kernel.h          # 内核公共头文件

├── user/                  # 用户态程序（未来）

│   └── test.c            # 用户测试程序

├── LICENSE                # MIT许可证

├── linker.ld              # 内核链接脚本

├── user.ld                # 用户程序链接脚本

├── Makefile               # 主构建文件

└── Makefile.user          # 用户程序构建文件
## 🚀 快速开始

### 环境准备
你需要一个类Unix开发环境（Linux/macOS/WSL2）并安装以下工具：
bash

Debian/Ubuntu

sudo apt-get install build-essential gcc-multilib nasm qemu-system-x86
bash

编译内核

make all

在QEMU中运行（会显示启动信息）

make run
如果运行成功，你将看到QEMU窗口输出 `huilin-Helios Boot Success!` 和定时器计数的信息。

## 📄 许可证
本项目采用 **MIT 许可证**。详情请见 [LICENSE](LICENSE) 文件。

---

*最后更新: 2026-05-06*
*内核版本: 0.1.0-prealpha*