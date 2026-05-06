#ifndef VMM_H
#define VMM_H

#include "system.h"
#include "paging.h"

void init_vmm(void);
void vmm_map_page(void* vaddr, void* paddr, uint32_t flags);
void vmm_unmap_page(void* vaddr);
page_directory_t* vmm_get_kernel_directory(void);   // 新增

#endif