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
#include "amdgv_misc.h"

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
 * amdgpu_gart_invalidate_tlb - invalidate gart TLB
 *
 * @adev: amdgpu device driver pointer
 *
 * Invalidate gart TLB which can be use as a way to flush gart changes
 *
 */
void amdgv_gart_invalidate_tlb(struct amdgv_adapter *adapt)
{
	int i;

	oss_mb();
	amdgv_misc_hdp_flush(adapt);
	for (i = 0; i < AMDGV_MAX_VMHUBS; i++)
		amdgv_gmc_flush_gpu_tlb(adapt, 0, i, 0);
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
	uint64_t flags = 0;
	unsigned t;
	int i;
	void *ptb_cpu_addr = amdgv_memmgr_get_cpu_addr(adapt->ptb_mem);

	if (adapt->gmc.funcs && adapt->gmc.funcs->get_gart_map_flags)
		flags = adapt->gmc.funcs->get_gart_map_flags(adapt);

	t = offset >> AMDGV_GPU_PAGE_SHIFT;

	for (i = 0; i < pages; i++) {
		AMDGV_DEBUG("GART address: 0x%llx DMA address: 0x%llx\n", (offset + (i << AMDGV_GPU_PAGE_SHIFT)), dma_addr + (i << AMDGV_GPU_PAGE_SHIFT));
		amdgv_gart_set_pte_pde(adapt, ptb_cpu_addr, t + i, dma_addr + (i << AMDGV_GPU_PAGE_SHIFT), flags);
	}

	if (!in_whole_gpu_reset())
		amdgv_gart_invalidate_tlb(adapt);
}

void amdgv_gart_init_pdb0(struct amdgv_adapter *adapt)
{
	uint64_t flags;
	void *pdb0_cpu_addr = amdgv_memmgr_get_cpu_addr(adapt->pdb0_mem);
	uint64_t ptb_pa = amdgv_memmgr_get_gpu_pa(adapt->ptb_mem);

	/* The first PDE0 entry points to a huge
	 * PTB who has more than 512 entries each
	 * pointing to a 4K system page.
	 * It is used for maping PF used memory.
	 * The others PDE0 entries are reserved for
	 * mapping VF memory and CXL memory in the future.
	 */
	flags = AMDGV_PTE_VALID;
	flags |= AMDGV_PTE_SNOOPED;
	amdgv_gart_set_pte_pde(adapt, pdb0_cpu_addr, 0, ptb_pa, flags);
}