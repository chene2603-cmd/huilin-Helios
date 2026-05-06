; kernel/switch.asm
; 进程上下文切换（内核线程版本，不涉及用户态/内核态特权级切换）
; void switch_process(pcb_t *next);

global switch_process
extern get_current_process

switch_process:
    ; 保存当前内核上下文到当前进程 PCB
    call get_current_process
    test eax, eax
    jz  .load_new              ; 如果没有当前进程，直接加载新进程

    ; 当前进程 PCB 指针在 eax 中
    ; 将通用寄存器压栈，保存顺序与 process.h 中 context 结构一致
    push ebx
    push ecx
    push edx
    push esi
    push edi
    push ebp

    ; 现在栈顶是保存的寄存器，我们将当前 esp 保存到 PCB 中
    mov [eax + 0], esp          ; 假设 PCB 第一个字段是 kernel_esp

    ; 保存 eflags
    pushfd
    pop dword [eax + 4]         ; 假设第二个字段是 eflags

    ; 保存页目录物理地址
    mov edx, cr3
    mov [eax + 8], edx          ; 假设第三个字段是 page_dir_phys

.load_new:
    ; 参数 next 在栈中：因为从这里开始我们可能改变了 esp，但参数是通过栈传递的
    ; 通过 [esp + offset] 获取 next 参数（根据之前压栈情况计算）
    mov eax, [esp + 28]         ; 6 个寄存器压栈 24 字节 + 返回地址 4 字节 = 28
    test eax, eax
    jz  .halt

    ; 切换页目录
    mov edx, [eax + 8]          ; page_dir_phys
    mov cr3, edx

    ; 恢复内核栈指针
    mov esp, [eax + 0]          ; kernel_esp

    ; 恢复通用寄存器（按照保存顺序反向弹出）
    pop ebp
    pop edi
    pop esi
    pop edx
    pop ecx
    pop ebx

    ; 恢复 eflags
    push dword [eax + 4]
    popfd

    ; 跳转到新进程的 EIP（由调用 switch_process 时的返回地址决定）
    ret                         ; 弹出栈顶的返回地址并跳转

.halt:
    cli
    hlt
    jmp .halt