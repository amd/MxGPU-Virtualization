/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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
 * Invalidate gart TLB which can be use as a way to flush gart changes
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
 *
 * NOTE: this function does NOT flush the GART TLB. The caller must call
 * amdgv_gart_invalidate_tlb() after amdgv_gart_map() (typically once after
 * a batch of map calls) before the new mappings are observed by the GPU.
 */
void amdgv_gart_map(struct amdgv_adapter *adapt, uint64_t offset, int pages,
		    uint64_t dma_addr)
{
	uint64_t flags = 0;
	unsigned t;
	int i;
	void *ptb_cpu_addr = amdgv_memmgr_get_cpu_addr(adapt->ptb_mem);

	if (adapt->gmc.funcs && adapt->gmc.funcs->get_gart_map_flags && 0 != dma_addr)
		flags = adapt->gmc.funcs->get_gart_map_flags(adapt);

	t = offset >> AMDGV_GPU_PAGE_SHIFT;

	for (i = 0; i < pages; i++) {
		AMDGV_DEBUG("GART address: 0x%llx DMA address: 0x%llx\n",
			    (offset + (i << AMDGV_GPU_PAGE_SHIFT)),
			    dma_addr + (i << AMDGV_GPU_PAGE_SHIFT));
		amdgv_gart_set_pte_pde(adapt, ptb_cpu_addr, t + i,
				       dma_addr + (i << AMDGV_GPU_PAGE_SHIFT), flags);
	}
}

void amdgv_gart_init_pdb0(struct amdgv_adapter *adapt)
{
	int i = 0;
	uint64_t flags = 0;
	uint64_t vram_size = adapt->fb_size;
	uint64_t pde0_page_size = AMDGV_PDE0_PAGE_SIZE;
	uint64_t vram_addr = adapt->fb_pa;
	uint64_t vram_end = vram_addr + vram_size;
	void *pdb0_cpu_addr = amdgv_memmgr_get_cpu_addr(adapt->pdb0_mem);
	uint64_t ptb_pa = amdgv_memmgr_get_gpu_pa(adapt->ptb_mem);

	if (adapt->xgmi.connected_to_cpu) {
		if (adapt->gmc.funcs && adapt->gmc.funcs->get_gart_map_flags)
			flags = adapt->gmc.funcs->get_gart_map_flags(adapt);
		flags &= ~AMDGV_PTE_SYSTEM;
		flags |= AMDGV_PTE_FRAG(AMDGV_PDE0_PAGE_SHIFT - AMDGV_GPU_PAGE_SHIFT);
		flags |= AMDGV_PDE_PTE_FLAG(adapt);

		/* First n PDE0 entries for VRAM, n+1'th for PTB */
		for (i = 0; vram_addr < vram_end; i++, vram_addr += pde0_page_size)
			amdgv_gart_set_pte_pde(adapt, pdb0_cpu_addr, i, vram_addr, flags);
	}

	/* PTB: for xgmi connected_to_cpu use the n+1'th PDE0; otherwise the first PDE0 */
	flags = AMDGV_PTE_VALID;
	flags |= AMDGV_PTE_SNOOPED | AMDGV_PDE_BFS_FLAG(adapt, 0);
	amdgv_gart_set_pte_pde(adapt, pdb0_cpu_addr, i, ptb_pa, flags);
}