/*
 * Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE
 */

#include <amdgv.h>
#include <amdgv_device.h>
#include <amdgv_gart.h>
#include <amdgv_gpuiov.h>
#include <amdgv_memmgr.h>
#include <amdgv_wb_memory.h>

#include "mi300.h"
#include "mi300/GC/gc_9_4_3_offset.h"
#include "mi300/GC/gc_9_4_3_sh_mask.h"
#include "mi300_nbio.h"
#include "gfxhub_v1_2.h"
#include "mmhub_v1_8.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

/* Default size is 256MB for the manager */
#define DEFAULT_MEMORY_MANAGER_SIZE (256 << 20)
#define DEFAULT_MEMORY_MANAGER_SIZE_MI350 (128 << 20)

#define AMDGV_PTE_MTYPE_GFX9(a)	((uint64_t)(a) << 57)

static uint32_t gmc_v9_0_get_invalidate_req(unsigned int vmid,
					uint32_t flush_type)
{
	uint32_t req = 0;

	req = REG_SET_FIELD(req, VM_INVALIDATE_ENG0_REQ,
			    PER_VMID_INVALIDATE_REQ, 1 << vmid);
	req = REG_SET_FIELD(req, VM_INVALIDATE_ENG0_REQ, FLUSH_TYPE, flush_type);
	req = REG_SET_FIELD(req, VM_INVALIDATE_ENG0_REQ, INVALIDATE_L2_PTES, 1);
	req = REG_SET_FIELD(req, VM_INVALIDATE_ENG0_REQ, INVALIDATE_L2_PDE0, 1);
	req = REG_SET_FIELD(req, VM_INVALIDATE_ENG0_REQ, INVALIDATE_L2_PDE1, 1);
	req = REG_SET_FIELD(req, VM_INVALIDATE_ENG0_REQ, INVALIDATE_L2_PDE2, 1);
	req = REG_SET_FIELD(req, VM_INVALIDATE_ENG0_REQ, INVALIDATE_L1_PTES, 1);
	req = REG_SET_FIELD(req, VM_INVALIDATE_ENG0_REQ,
			    CLEAR_PROTECTION_FAULT_STATUS_ADDR,	0);

	return req;
}

static int gmc_v9_0_flush_gpu_tlb(struct amdgv_adapter *adapt, uint32_t vmid,
					uint32_t vmhub, uint32_t flush_type)
{
	uint32_t inv_req, req, ack;
	const unsigned int eng = 17;
	struct amdgv_vmhub *hub;
	int ret = 0;

	hub = &adapt->vmhub[vmhub];
	inv_req = gmc_v9_0_get_invalidate_req(vmid, flush_type);
	req = hub->vm_inv_eng0_req + hub->eng_distance * eng;
	ack = hub->vm_inv_eng0_ack + hub->eng_distance * eng;

	if (!req)
		return ret;

	WREG32(req, inv_req);
	AMDGV_DEBUG("req:%x, ack:%x, inv_req:%x\n", req, ack, inv_req);

	ret = amdgv_wait_for_register(
				adapt, ack,
				0xFFFFFFFF, 1 << vmid,
				AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ,
				AMDGV_WAIT_FLAG_AUTO);

	if (ret)
		AMDGV_ERROR("Timeout waiting for VM flush ACK!, ack:%x\n", RREG32(ack));

	return ret;
}

static uint64_t gmc_v9_0_get_gart_map_flags(struct amdgv_adapter *adapt)
{
	uint64_t flags = 0;

	flags = AMDGV_PTE_MTYPE_GFX9(MTYPE_UC);
	flags |= AMDGV_PTE_EXECUTABLE;
	flags |= AMDGV_PTE_VALID;
	flags |= AMDGV_PTE_READABLE;
	flags |= AMDGV_PTE_WRITEABLE;
	flags |= AMDGV_PTE_SNOOPED;
	flags |= AMDGV_PTE_SYSTEM;

	return flags;
}

static const struct amdgv_gmc_funcs gmc_v9_0_gmc_funcs = {
	.flush_gpu_tlb = gmc_v9_0_flush_gpu_tlb,
	.get_gart_map_flags = gmc_v9_0_get_gart_map_flags,
};

static void gmc_v9_0_set_gmc_funcs(struct amdgv_adapter *adapt)
{
	adapt->gmc.funcs = &gmc_v9_0_gmc_funcs;
}

static int mi300_mem_sw_init(struct amdgv_adapter *adapt)
{
	uint64_t libgv_res_fb_offset;
	uint64_t libgv_res_fb_size;
	uint32_t ptb_table_size;

	gfxhub_v1_2_init(adapt);
	mmhub_v1_8_init(adapt);
	gmc_v9_0_set_gmc_funcs(adapt);

	/* Use hypervisor's configuration to allocate FB for PF memmgr */
	if (adapt->opt.libgv_res_fb_size != AMDGV_USE_DEFAULT_MEMMGR) {
		libgv_res_fb_offset = adapt->opt.libgv_res_fb_offset;
		libgv_res_fb_size = adapt->opt.libgv_res_fb_size;
	} else {
		libgv_res_fb_offset = 0x0;
		libgv_res_fb_size = DEFAULT_MEMORY_MANAGER_SIZE;
	}

	/* Allocate a memory manager for the PF Framebuffer */
	if (amdgv_memmgr_init(adapt, &adapt->memmgr_pf, libgv_res_fb_offset, libgv_res_fb_size,
			      0, false)) {
		AMDGV_ERROR("Failed to init PF FB memory manager\n");
		goto pf_fail;
	}

	if (amdgv_wb_memory_init(adapt)) {
		AMDGV_ERROR("Failed to init GPU FB WB memory\n");
		goto wb_fail;
	}

	adapt->gart_size = GART_SIZE;

	if (amdgv_memmgr_init(adapt, &adapt->memmgr_sys, GART_START, adapt->gart_size,
			      0, false)) {
		AMDGV_ERROR("Failed to init system memory manager\n");
		goto sys_fail;
	}
	adapt->memmgr_sys.is_sys = true;
	amdgv_memmgr_set_gpu_base(&adapt->memmgr_sys, GART_START);

	if (amdgv_memmgr_init(adapt, &adapt->memmgr_gpu, 0, DEFAULT_MEMORY_MANAGER_SIZE, 0, true)) {
			AMDGV_ERROR("Failed to init GPU FB memory manager\n");
			goto gpu_fail;
	}

	AMDGV_DEBUG("Alloc memory for gart table\n");

	/* alloccate 4K dma memory for pdb0 */
	adapt->pdb0_mem = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, 0x1000,
							 PAGE_SIZE, MEM_GART_MEM_PDB0);
	if (!adapt->pdb0_mem != 0) {
		AMDGV_ERROR("Failed to allocate DMA memory for pdb0!\n");
		goto pdb0_mem_fail;
	}

	/* each PTE is 8 bytes */
	ptb_table_size = (adapt->gart_size >> AMDGV_GPU_PAGE_SHIFT) * 8;
	adapt->ptb_mem = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, ptb_table_size,
							 PAGE_SIZE, MEM_GART_MEM_PTB);
	if (!adapt->ptb_mem) {
		AMDGV_ERROR("Failed to allocate DMA memory for ptb!\n");
		goto ptb_mem_fail;
	}
	return 0;

ptb_mem_fail:
	amdgv_memmgr_free(adapt->pdb0_mem);
pdb0_mem_fail:
	amdgv_memmgr_fini(adapt, &adapt->memmgr_gpu);
gpu_fail:
	amdgv_memmgr_fini(adapt, &adapt->memmgr_sys);
sys_fail:
	amdgv_wb_memory_fini(adapt);
wb_fail:
	amdgv_memmgr_fini(adapt, &adapt->memmgr_pf);
pf_fail:
	return AMDGV_FAILURE;
}

static int mi300_mem_sw_fini(struct amdgv_adapter *adapt)
{
	amdgv_memmgr_free(adapt->pdb0_mem);
	amdgv_memmgr_free(adapt->ptb_mem);
	amdgv_wb_memory_fini(adapt);
	amdgv_memmgr_fini(adapt, &adapt->memmgr_pf);
	amdgv_memmgr_fini(adapt, &adapt->memmgr_sys);
	amdgv_memmgr_fini(adapt, &adapt->memmgr_gpu);

	return 0;
}

static int mi300_mem_gart_enable(struct amdgv_adapter *adapt)
{
	int ret = 0;

	AMDGV_INFO("pdb0: VA: %p PA: %llx BA: %llx\n", amdgv_memmgr_get_cpu_addr(adapt->pdb0_mem),
	    amdgv_memmgr_get_gpu_pa(adapt->pdb0_mem), amdgv_memmgr_get_gpu_addr(adapt->pdb0_mem));

	AMDGV_INFO("ptb: VA: %p PA: %llx BA: %llx\n", amdgv_memmgr_get_cpu_addr(adapt->ptb_mem),
	    amdgv_memmgr_get_gpu_pa(adapt->ptb_mem), amdgv_memmgr_get_gpu_addr(adapt->ptb_mem));

	amdgv_gart_init_pdb0(adapt);

	gfxhub_v1_2_gart_enable(adapt);
	mmhub_v1_8_gart_enable(adapt);

	adapt->gart_ready = true;

	ret = amdgv_map_sys_mem_allocs(&adapt->memmgr_sys, AMDGV_MAP);

	return ret;
}

static int mi300_mem_hw_init(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	uint64_t mc_fb_loc_base;
	uint64_t offset;
	int ret = 0;

	tmp = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, 0), regMC_VM_FB_LOCATION_BASE));
	tmp = REG_GET_FIELD(tmp, MC_VM_FB_LOCATION_BASE, FB_BASE);

	mc_fb_loc_base = (uint64_t)tmp;
	mc_fb_loc_base = mc_fb_loc_base << MC_VM_FB_LOCATION__FB_ADDRESS__SHIFT;

	/*Add xgmi offset while initializing memory manager*/
	offset = adapt->xgmi.phy_node_id * adapt->xgmi.node_segment_size;

	/* Initialize Manager at the base of FB */
	amdgv_memmgr_set_gpu_base(&adapt->memmgr_pf, mc_fb_loc_base + offset);
	amdgv_memmgr_set_cpu_base(&adapt->memmgr_pf, adapt->fb);
	AMDGV_DEBUG("memmgr_pf gpu base at: 0x%llx\n", mc_fb_loc_base + offset);

	ret = mi300_mem_gart_enable(adapt);
	if (ret)
		return ret;

	amdgv_wb_memory_hw_init_address(adapt);
	amdgv_wb_memory_clear(adapt);

	return ret;
}

static int mi300_mem_hw_fini(struct amdgv_adapter *adapt)
{
	amdgv_map_sys_mem_allocs(&adapt->memmgr_sys, AMDGV_UNMAP);

	adapt->gart_ready = false;
	gfxhub_v1_2_gart_fini(adapt);
	mmhub_v1_8_gart_fini(adapt);
	return 0;
}

static int mi300_mem_hw_live_fini(struct amdgv_adapter *adapt)
{
	adapt->gart_ready = false;

	amdgv_map_sys_mem_allocs(&adapt->memmgr_sys, AMDGV_UNMAP);

	return 0;
}

const struct amdgv_init_func mi300_mem_func = {
	.name = "mi300_mem_func",
	.sw_init = mi300_mem_sw_init,
	.sw_fini = mi300_mem_sw_fini,
	.hw_init = mi300_mem_hw_init,
	.hw_fini = mi300_mem_hw_fini,
	.hw_live_init = mi300_mem_hw_init,
	.hw_live_fini = mi300_mem_hw_live_fini
};

static int mi350_mem_sw_init(struct amdgv_adapter *adapt)
{
	uint64_t libgv_res_fb_offset;
	uint64_t libgv_res_fb_size;
	uint32_t ptb_table_size;

	gfxhub_v1_2_init(adapt);
	mmhub_v1_8_init(adapt);
	gmc_v9_0_set_gmc_funcs(adapt);

	/* Use hypervisor's configuration to allocate FB for PF memmgr */
	if (adapt->opt.libgv_res_fb_size != AMDGV_USE_DEFAULT_MEMMGR) {
		libgv_res_fb_offset = adapt->opt.libgv_res_fb_offset;
		libgv_res_fb_size = adapt->opt.libgv_res_fb_size;
	} else {
		/* Set memmgr_pf to 128MB in advanced */
		libgv_res_fb_offset = 0x0;
		libgv_res_fb_size = DEFAULT_MEMORY_MANAGER_SIZE_MI350;
	}

	/* Allocate a memory manager for the PF Framebuffer */
	if (amdgv_memmgr_init(adapt, &adapt->memmgr_pf, libgv_res_fb_offset, libgv_res_fb_size,
			      0, false)) {
		AMDGV_ERROR("Failed to init PF FB memory manager\n");
		goto pf_fail;
	}

	if (amdgv_wb_memory_init(adapt)) {
		AMDGV_ERROR("Failed to init GPU FB WB memory\n");
		goto wb_fail;
	}

	adapt->gart_size = GART_SIZE;

	if (amdgv_memmgr_init(adapt, &adapt->memmgr_sys, GART_START, adapt->gart_size,
			      0, false)) {
		AMDGV_ERROR("Failed to init system memory manager\n");
		goto sys_fail;
	}
	adapt->memmgr_sys.is_sys = true;
	amdgv_memmgr_set_gpu_base(&adapt->memmgr_sys, GART_START);

	if (amdgv_memmgr_init(adapt, &adapt->memmgr_gpu, 0, DEFAULT_MEMORY_MANAGER_SIZE_MI350, 0, true)) {
			AMDGV_ERROR("Failed to init GPU FB memory manager\n");
			goto gpu_fail;
	}

	AMDGV_DEBUG("Store attributes for gart table, the memory allocation is deferred until we get bp info.\n");
	/* alloccate 4K dma memory for pdb0 */
	adapt->pdb0_mem = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, 0x1000,
							 PAGE_SIZE, MEM_GART_MEM_PDB0);
	if (!adapt->pdb0_mem) {
		AMDGV_ERROR("Failed to store attributes for pdb0!\n");
		goto pdb0_mem_fail;
	}

	/* each PTE is 8 bytes */
	ptb_table_size = (adapt->gart_size >> AMDGV_GPU_PAGE_SHIFT) * 8;
	adapt->ptb_mem = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, ptb_table_size,
							 PAGE_SIZE, MEM_GART_MEM_PTB);
	if (!adapt->ptb_mem) {
		AMDGV_ERROR("Failed to store attributes for ptb!\n");
		goto ptb_mem_fail;
	}
	return 0;

ptb_mem_fail:
	amdgv_memmgr_free(adapt->pdb0_mem);
pdb0_mem_fail:
	amdgv_memmgr_fini(adapt, &adapt->memmgr_gpu);
gpu_fail:
	amdgv_memmgr_fini(adapt, &adapt->memmgr_sys);
sys_fail:
	amdgv_wb_memory_fini(adapt);
wb_fail:
	amdgv_memmgr_fini(adapt, &adapt->memmgr_pf);
pf_fail:
	return AMDGV_FAILURE;
}

static int mi350_mem_sw_fini(struct amdgv_adapter *adapt)
{
	amdgv_memmgr_free(adapt->pdb0_mem);
	amdgv_memmgr_free(adapt->ptb_mem);
	amdgv_wb_memory_fini(adapt);
	amdgv_memmgr_fini(adapt, &adapt->memmgr_pf);
	amdgv_memmgr_fini(adapt, &adapt->memmgr_sys);
	amdgv_memmgr_fini(adapt, &adapt->memmgr_gpu);

	return 0;
}

static int mi350_mem_hw_init(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	uint64_t mc_fb_loc_base;
	uint64_t offset;
	int ret = 0;
	uint64_t *bp_offsets = NULL;
	int bp_idx = 0;
	int bp_count = 0;
	uint64_t pf_fb_offset = 0, pf_fb_size = 0;
	struct amdgv_memmgr_mem *alloc = NULL;

	if (!adapt->memmgr_pf.is_init || !adapt->memmgr_gpu.is_init)
		return AMDGV_FAILURE;

	pf_fb_offset = adapt->memmgr_pf.offset;
	pf_fb_size = adapt->memmgr_pf.size;

	/* Add allocation for bad pages in all supported NPS mode */
	if (adapt->eeprom_control.num_recs && adapt->ecc.eh_data_across_nps_initialized) {
		if (amdgv_umc_fetch_and_sort_bps_across_nps(adapt, &bp_offsets, &bp_count)) {
			return AMDGV_FAILURE;
		}

		if (bp_count == 0)
			goto free_buf;
		for (bp_idx = 0; bp_idx < bp_count; bp_idx++) {
			if (bp_offsets[bp_idx] + PAGE_SIZE < pf_fb_offset)
				continue;
			else if (bp_offsets[bp_idx] >= pf_fb_offset + pf_fb_size) {
				break;
			}

			alloc = amdgv_memmgr_alloc_align_at(&adapt->memmgr_pf,
							bp_offsets[bp_idx], PAGE_SIZE,
							MEM_ECC_BAD_PAGE_NPS_CANDIDATE);

			if (!alloc) {
				AMDGV_ERROR("Allocate cross-nps bad page into PF critical region failed!\n");
				ret = AMDGV_FAILURE;
				goto free_buf;
			}
		}
	}

	/* Retrieve the entry from reserve list then allocate to alloc list in size-descending order */
	if (amdgv_memmgr_alloc_deferred_region(&adapt->memmgr_pf) || amdgv_memmgr_alloc_deferred_region(&adapt->memmgr_gpu)) {
		return AMDGV_FAILURE;
	}

	/* Free previous allocation on MEM_ECC_BAD_PAGE_NPS_CANDIDATE */
	if (alloc) {
		if (amdgv_memmgr_free_by_id(&adapt->memmgr_pf, MEM_ECC_BAD_PAGE_NPS_CANDIDATE))
			return AMDGV_FAILURE;
	}

	tmp = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, 0), regMC_VM_FB_LOCATION_BASE));
	tmp = REG_GET_FIELD(tmp, MC_VM_FB_LOCATION_BASE, FB_BASE);

	mc_fb_loc_base = (uint64_t)tmp;
	mc_fb_loc_base = mc_fb_loc_base << MC_VM_FB_LOCATION__FB_ADDRESS__SHIFT;

	/*Add xgmi offset while initializing memory manager*/
	offset = adapt->xgmi.phy_node_id * adapt->xgmi.node_segment_size;

	/* Initialize Manager at the base of FB */
	amdgv_memmgr_set_gpu_base(&adapt->memmgr_pf, mc_fb_loc_base + offset);
	amdgv_memmgr_set_cpu_base(&adapt->memmgr_pf, adapt->fb);
	AMDGV_DEBUG("memmgr_pf gpu base at: 0x%llx\n", mc_fb_loc_base + offset);

	ret = mi300_mem_gart_enable(adapt);
	if (ret)
		return ret;

	amdgv_wb_memory_hw_init_address(adapt);
	amdgv_wb_memory_clear(adapt);

free_buf:
	if (bp_offsets)
		oss_free(bp_offsets);

	return ret;
}

static int mi350_mem_hw_fini(struct amdgv_adapter *adapt)
{
	amdgv_map_sys_mem_allocs(&adapt->memmgr_sys, AMDGV_UNMAP);

	adapt->gart_ready = false;
	gfxhub_v1_2_gart_fini(adapt);
	mmhub_v1_8_gart_fini(adapt);
	return 0;
}

const struct amdgv_init_func mi350_mem_func = {
	.name = "mi350_mem_func",
	.sw_init = mi350_mem_sw_init,
	.sw_fini = mi350_mem_sw_fini,
	.hw_init = mi350_mem_hw_init,
	.hw_fini = mi350_mem_hw_fini,
	.hw_live_init = mi300_mem_hw_init,
	.hw_live_fini = mi300_mem_hw_live_fini
};