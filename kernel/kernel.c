// 内核主函数（完整版）
void kernel_main(multiboot_info_t* mbi, uint32_t magic) {
    // 初始化屏幕
    screen_init();
    screen_clear();
    
    // 检查多引导魔数
    if (magic != 0x2BADB002) {
        screen_setcolor(COLOR_LIGHT_RED, COLOR_BLACK);
        screen_putstr("ERROR: Invalid multiboot magic number: ");
        screen_puthex(magic);
        screen_putstr("\n");
        return;
    }
    
    mb_info = mbi;
    
    // 显示启动信息
    show_banner();
    
    // 检测内存
    detect_memory();
    
    // 初始化物理内存管理器
    init_pmm(mb_info->mem_upper + mb_info->mem_lower);
    
    // 初始化分页
    init_paging();
    
    // 初始化内核堆
    init_kheap(0xE0000000, 0x10000000);  // 3.5GB开始的256MB
    
    // 初始化中断
    screen_putstr("\n");
    init_idt();
    init_pic();
    
    // 初始化定时器
    init_timer(100);  // 100Hz
    
    // 初始化设备
    init_devices();
    
    // 初始化文件系统
    init_fs();
    
    // 初始化ATA控制器
    init_ata();
    
    // 初始化控制台
    init_console();
    
    // 初始化系统调用
    init_syscalls();
    
    // 初始化进程管理器
    init_process_manager();
    
    // 初始化调度器
    init_scheduler();
    
    // 启用中断
    asm volatile("sti");
    
    screen_setcolor(COLOR_LIGHT_GREEN, COLOR_BLACK);
    screen_putstr("\nAll subsystems initialized successfully!\n");
    
    // 创建初始进程
    pcb_t* shell_proc = create_process("shell", (void*)0x400000);
    if (shell_proc) {
        screen_putstr("Created shell process (PID=");
        screen_putdec(shell_proc->pid);
        screen_putstr(")\n");
        add_to_ready_queue(shell_proc);
    }
    
    screen_setcolor(COLOR_LIGHT_CYAN, COLOR_BLACK);
    screen_putstr("\n========================================\n");
    screen_putstr("  Helios OS v0.1.0 - Ready\n");
    screen_putstr("========================================\n\n");
    
    // 显示系统信息
    show_sysinfo();
    
    // 进入主控制台循环
    while (1) {
        // 处理控制台输入
        console_process();
        
        // 简单延时
        for (volatile int i = 0; i < 10000; i++);
    }
}

// 显示系统信息
void show_sysinfo(void) {
    screen_setcolor(COLOR_LIGHT_GREY, COLOR_BLACK);
    screen_putstr("System Information:\n");
    screen_putstr("-------------------\n");
    
    // 内存信息
    pmm_info_t mem_info;
    get_pmm_info(&mem_info);
    
    screen_putstr("Memory: ");
    screen_putdec(mem_info.total_memory / 1024 / 1024);
    screen_putstr(" MB total, ");
    screen_putdec(mem_info.free_pages * PAGE_SIZE / 1024);
    screen_putstr(" KB free\n");
    
    // 进程信息
    pcb_t* current = get_current_process();
    if (current) {
        screen_putstr("Current process: ");
        screen_putstr(current->name);
        screen_putstr(" (PID=");
        screen_putdec(current->pid);
        screen_putstr(")\n");
    }
    
    // 时间信息
    screen_putstr("Uptime: ");
    screen_putdec(get_time_ms() / 1000);
    screen_putstr(" seconds\n");
    
    // 系统调用
    screen_putstr("System calls: ");
    screen_putdec(20);
    screen_putstr(" registered\n");
    
    // 设备信息
    screen_putstr("Devices: 3 registered (console, null, zero)\n");
    
    screen_putstr("\nType 'help' for available commands\n");
    console_prompt();
}