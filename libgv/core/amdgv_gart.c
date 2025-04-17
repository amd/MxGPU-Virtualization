/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <amdgv.h>
#include <amdgv_device.h>
#include "amdgv_gart.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

/**
 * amdgv_gart_set_pte_pde - update the page tables using CPU
 *
 * @adapt: amdgv_adapter pointer
 * @cpu_pt_addr: cpu address of the page table
 * @gpu_page_idx: entry in the page table to update
 * @dma_addr: dma dma_addr to write into pte/pde
 * @flags: access flags
 */
static int amdgv_gart_set_pte_pde(struct amdgv_adapter *adapt, void *cpu_pt_addr,
				uint32_t gpu_page_idx, uint64_t dma_addr,
				uint64_t flags)
{
	uint64_t value;

	value = dma_addr & 0x0000FFFFFFFFF000ULL;
	value |= flags;
	oss_mm_write64((char *)cpu_pt_addr + (gpu_page_idx * 8), value);

	return 0;
}

/**
 * amdgv_gart_map - map dma addresses into GART entries
 *
 * @adapt: amdgv_adapter pointer
 * @offset: offset into the GPU's gart aperture
 * @pages: number of pages to bind
 * @dma_addr: dma address of the pages
 */
void amdgv_gart_map(struct amdgv_adapter *adapt, uint64_t offset, int pages,
		    uint64_t dma_addr)
{
	uint64_t flags;
	unsigned i, t;
	void *ptb_cpu_addr = adapt->ptb_mem.va_ptr;

	flags = AMDGV_PTE_MTYPE_GFX9(MTYPE_UC);
	flags |= AMDGV_PTE_EXECUTABLE;
	flags |= AMDGV_PTE_VALID;
	flags |= AMDGV_PTE_READABLE;
	flags |= AMDGV_PTE_WRITEABLE;
	flags |= AMDGV_PTE_SNOOPED;
	flags |= AMDGV_PTE_SYSTEM;

	t = offset >> AMDGV_GPU_PAGE_SHIFT;

	for (i = 0; i < pages; i++) {
		AMDGV_DEBUG("GART address: 0x%llx DMA address: 0x%llx\n", (offset + (i << AMDGV_GPU_PAGE_SHIFT)), dma_addr + (i << AMDGV_GPU_PAGE_SHIFT));
		amdgv_gart_set_pte_pde(adapt, ptb_cpu_addr, t + i, dma_addr + (i << AMDGV_GPU_PAGE_SHIFT), flags);
	}
}

void amdgv_gart_init_pdb0(struct amdgv_adapter *adapt)
{
	uint64_t flags;
	void *pdb0_cpu_addr = adapt->pdb0_mem.va_ptr;
	uint64_t gart_dma_addr = adapt->ptb_mem.bus_addr;

	/* The first PDE0 entry points to a huge
	 * PTB who has more than 512 entries each
	 * pointing to a 4K system page.
	 * It is used for maping PF used memory.
	 * The others PDE0 entries are reserved for
	 * mapping VF memory and CXL memory in the future.
	 */
	flags = AMDGV_PTE_VALID;
	flags |= AMDGV_PTE_SYSTEM;
	flags |= AMDGV_PTE_SNOOPED;
	amdgv_gart_set_pte_pde(adapt, pdb0_cpu_addr, 0, gart_dma_addr, flags);
}