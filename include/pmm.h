#ifndef PMM_H
#define PMM_H

#include "system.h"

void init_pmm(uint32_t mem_size);
void* pmm_alloc_page(void);
void pmm_free_page(void* page);

#endif