typedef struct {
    uint32_t esp;
    uint32_t eflags;
    uint32_t page_dir_phys;
} switch_context_t;

typedef struct process_control_block {
    switch_context_t ctx;       // 偏移 0~8
    // ... 其他字段
} pcb_t;