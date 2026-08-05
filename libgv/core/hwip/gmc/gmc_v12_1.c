/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv.h>
#include <amdgv_device.h>
#include <amdgv_gart.h>
#include <amdgv_nbio.h>
#include <amdgv_memmgr.h>
#include <amdgv_oss_wrapper.h>

#include "mmhub/mmhub_v4_2_0.h"
#include "gfxhub/gfxhub_v12_1_0.h"


static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

#define DEFAULT_MEMORY_MANAGER_SIZE (256 << 20)
#define GMC_V12_1_FB_ALIGNMENT 256

#define AMDGV_PTE_MTYPE_GFX12(a)	((uint64_t)(a) << 54)

static int gmc_v12_1_flush_gpu_tlb(struct amdgv_adapter *adapt, uint32_t vmid,
			uint32_t vmhub, uint32_t flush_type)
{
	uint32_t inv_req, req, ack;
	/* Register 17 for GART */
	const unsigned int eng = 17;
	struct amdgv_vmhub *hub;
	int ret = 0;

	if ((vmhub >= AMDGV_GFXHUB_START) && (vmhub < AMDGV_MMHUB0_START) &&
	    amdgv_gfx_is_gfx_off(adapt))
		return 0;

	hub = &adapt->vmhub[vmhub];
	if (!hub->vmhub_funcs || !hub->vmhub_funcs->get_invalidate_req) {
		AMDGV_ERROR("Failed to get invalidate reg\n");
		return AMDGV_FAILURE;
	}

	inv_req = hub->vmhub_funcs->get_invalidate_req(vmid, flush_type);
	req = hub->vm_inv_eng0_req + hub->eng_distance * eng;
	ack = hub->vm_inv_eng0_ack + hub->eng_distance * eng;

	if (!req)
		return ret;

	WREG32(req, inv_req);
	AMDGV_DEBUG("req:%x, ack:%x, inv_req:%x\n", req, ack, inv_req);

	ret = amdgv_wait_for_register(
				adapt, ack, "GCVM_INVALIDATE_ENG_ACK",
				0xFFFFFFFF, 1 << vmid,
				AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ,
				AMDGV_WAIT_FLAG_AUTO);

	if (hub->vm_l2_bank_select_reserved_cid2) {
		inv_req = RREG32(hub->vm_l2_bank_select_reserved_cid2);
		inv_req |= (1 << 25);
		WREG32(hub->vm_l2_bank_select_reserved_cid2, inv_req);
		/* read back to ensure invalidation is done */
		RREG32(hub->vm_l2_bank_select_reserved_cid2);
	}

	if (ret)
		AMDGV_ERROR("Timeout waiting for VM flush ACK!, ack:%x\n", RREG32(ack));

	return ret;
}

static uint64_t gmc_v12_1_get_gart_map_flags(struct amdgv_adapter *adapt)
{
	uint64_t flags = 0;

	flags = AMDGV_PTE_MTYPE_GFX12(MTYPE_UC);
	flags |= AMDGV_PTE_EXECUTABLE;
	flags |= AMDGV_PTE_VALID;
	flags |= AMDGV_PTE_READABLE;
	flags |= AMDGV_PTE_WRITEABLE;
	flags |= AMDGV_PTE_SNOOPED;
	flags |= AMDGV_PTE_SYSTEM;

	return flags;
}

static const struct amdgv_gmc_funcs gmc_v12_1_gmc_funcs = {
	.flush_gpu_tlb = gmc_v12_1_flush_gpu_tlb,
	.get_gart_map_flags = gmc_v12_1_get_gart_map_flags,
};

static int gmc_v12_1_set_funcs(struct amdgv_adapter *adapt)
{
	adapt->gmc.funcs = &gmc_v12_1_gmc_funcs;

	return 0;
}

static int gmc_v12_1_get_mc_location_settings(struct amdgv_adapter *adapt)
{
	if (amdgv_mmhub_get_fb_location(adapt, &adapt->mc_fb_loc_addr, &adapt->mc_fb_top_addr))
		return AMDGV_FAILURE;

	adapt->mc_fb_top_addr |= ((1 << MC_VM_FB_LOCATION__FB_ADDRESS__SHIFT) - 1);

	if (!(adapt->flags & AMDGV_FLAG_USE_PF)) {
		adapt->mc_agp_loc_addr = adapt->mc_fb_top_addr + 1;
		adapt->mc_agp_top_addr = adapt->mc_agp_loc_addr + AMDGV_AGP_APERTURE_SIZE - 1;
	} else {
		adapt->mc_agp_top_addr = adapt->mc_agp_loc_addr = adapt->mc_fb_top_addr;
	}

	adapt->mc_sys_loc_addr = adapt->mc_fb_loc_addr;
	adapt->mc_sys_top_addr = adapt->mc_agp_top_addr;

	AMDGV_INFO("FB MC:  [0x%llx - 0x%llx]\n", adapt->mc_fb_loc_addr, adapt->mc_fb_top_addr);
	AMDGV_INFO("SYS MC: [0x%llx - 0x%llx]\n", adapt->mc_sys_loc_addr, adapt->mc_sys_top_addr);

	return 0;
}

static int gmc_v12_1_sw_init(struct amdgv_adapter *adapt)
{
	uint64_t ptb_table_size;

	if (adapt->xgmi.connected_to_cpu) {
		AMDGV_DEBUG("A + A memory mode, alloc memory for gart table\n");
		adapt->pdb0_mem = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, 0x1000,
								PAGE_SIZE, MEM_GART_MEM_PDB0);
		if (!adapt->pdb0_mem) {
			AMDGV_ERROR("Failed to allocate DMA memory for pdb0!\n");
			return AMDGV_FAILURE;
		}
		adapt->gart_size = GART_SIZE;
		ptb_table_size = (adapt->gart_size >> AMDGV_GPU_PAGE_SHIFT) * 8;
		adapt->ptb_mem = amdgv_memmgr_alloc_align(&adapt->memmgr_pf, ptb_table_size,
								PAGE_SIZE, MEM_GART_MEM_PTB);
		if (!adapt->ptb_mem) {
			AMDGV_ERROR("Failed to allocate DMA memory for ptb!\n");
			amdgv_memmgr_free(adapt->pdb0_mem);
			return AMDGV_FAILURE;
		}
	}

	adapt->fb_alignment = GMC_V12_1_FB_ALIGNMENT;

	oss_set_dma_mask(adapt->dev, 52);

	gmc_v12_1_set_funcs(adapt);

	if (amdgv_memmgr_init(adapt, &adapt->memmgr_gpu, 0, DEFAULT_MEMORY_MANAGER_SIZE, 0, true)) {
		AMDGV_ERROR("Failed to init GPU FB memory manager\n");
		return AMDGV_FAILURE;
	}

	if (!adapt->wb.wb_obj) {
		if (amdgv_wb_memory_init(adapt)) {
			AMDGV_ERROR("Failed to init WB memory\n");
			return AMDGV_FAILURE;
		}
	}

	return 0;
}

static int gmc_v12_1_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->xgmi.connected_to_cpu) {
		amdgv_memmgr_free(adapt->pdb0_mem);
		amdgv_memmgr_free(adapt->ptb_mem);
	}

	if (adapt->wb.wb_obj)
		amdgv_wb_memory_fini(adapt);

	amdgv_memmgr_fini(adapt, &adapt->memmgr_gpu);

	return 0;
}

static int gmc_v12_1_hw_init(struct amdgv_adapter *adapt)
{
	uint64_t offset, mem_size;

	if (adapt->xgmi.connected_to_cpu) {
		adapt->mc_fb_offset = amdgv_mmhub_get_mc_fb_offset(adapt) + adapt->xgmi.phy_node_id * adapt->xgmi.node_segment_size;
		AMDGV_DEBUG("mc_fb_offset: 0x%llx\n", adapt->mc_fb_offset);
		adapt->mc_fb_loc_addr = 0;
		adapt->mc_fb_top_addr = 0;
		if (adapt->fb_pa == 0) {
			adapt->fb_pa = adapt->mc_fb_offset;
			adapt->fb_size = adapt->xgmi.node_segment_size;
			adapt->fb = oss_memremap(adapt->fb_pa,
					adapt->fb_size,
					OSS_MEMREMAP_WB);
			if (!adapt->fb) {
				AMDGV_ERROR("Failed to map framebuffer memory\n");
				return AMDGV_FAILURE;
			}
		}

		/* gpu address (gart address) start from 0 */
		amdgv_memmgr_set_gpu_base(&adapt->memmgr_pf, GART_START);
		amdgv_memmgr_set_cpu_base(&adapt->memmgr_pf, adapt->fb);

		amdgv_memmgr_set_gpu_base(&adapt->memmgr_gpu, GART_START + adapt->fb_size);
		amdgv_memmgr_set_cpu_base(&adapt->memmgr_gpu, (void *)(((uint32_t *)adapt->fb) + (adapt->fb_size >> 2)));

		amdgv_gart_init_pdb0(adapt);
	} else {
		gmc_v12_1_get_mc_location_settings(adapt);
		offset = adapt->xgmi.phy_node_id * adapt->xgmi.node_segment_size;

		amdgv_memmgr_set_gpu_base(&adapt->memmgr_pf, adapt->mc_fb_loc_addr + offset);
		amdgv_memmgr_set_cpu_base(&adapt->memmgr_pf, adapt->fb);
		AMDGV_DEBUG("memmgr_pf gpu base at: 0x%llx\n", adapt->mc_fb_loc_addr + offset);

		mem_size = (uint64_t)amdgv_nbio_get_memsize(adapt);
		mem_size = MBYTES_TO_BYTES(mem_size);
		if (!mem_size)
			return AMDGV_FAILURE;

		amdgv_memmgr_set_gpu_base(&adapt->memmgr_gpu, adapt->mc_fb_loc_addr + mem_size);
		amdgv_memmgr_set_cpu_base(&adapt->memmgr_gpu, (void *)(((uint32_t *)adapt->fb) + (mem_size >> 2)));
		AMDGV_DEBUG("memmgr_gpu gpu base at: 0x%llx\n", adapt->mc_fb_loc_addr + mem_size);
	}

	if (adapt->wb.wb_obj) {
		if (amdgv_wb_memory_hw_init_address(adapt)) {
			AMDGV_ERROR("Failed to init WB memory address\n");
			return AMDGV_FAILURE;
		}
	}

	return 0;
}

static int gmc_v12_1_hw_fini(struct amdgv_adapter *adapt)
{

	return 0;
}

const struct amdgv_init_func gmc_v12_1_func = {
	.name = "gmc_v12_1_func",
	.sw_init = gmc_v12_1_sw_init,
	.sw_fini = gmc_v12_1_sw_fini,
	.hw_init = gmc_v12_1_hw_init,
	.hw_fini = gmc_v12_1_hw_fini,
};
