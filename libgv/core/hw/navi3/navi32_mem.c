/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv.h>
#include <amdgv_device.h>
#include <amdgv_gart.h>
#include <amdgv_gpuiov.h>
#include <amdgv_memmgr.h>
#include <amdgv_wb_memory.h>

#include "navi32_reg_inc.h"
#include "navi32_ip_discovery.h"
#include "mmhub_v3_0.h"
#include "gfx_v11_0.h"

#include <navi3/GC/gc_11_0_3_offset.h>
#include <navi3/GC/gc_11_0_3_sh_mask.h>

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

/* Default size is 256MB for the manager */
#define DEFAULT_MEMORY_MANAGER_SIZE (256 << 20)
/* PSP reserves 2MB TMR at the top of FB.
 * From NV21, VBIOS report total reserved FB (by PSPBL + VBIOS)
 * at firmwareInfoTable: "fw_reserved_size_in_kb"
 * Driver will get this table information on powerplay hw_init.
 * To simplify the logic, we hardcode the setting 3M here.
 * (2M for psp | 1M  for vbios)
 */
#define NAVI3_RESERVED_FB_SIZE (3 << 20)

#define AMDGV_PTE_MTYPE_GFX11(a)	((uint64_t)(a) << 48)

static int gmc_v11_0_flush_gpu_tlb(struct amdgv_adapter *adapt, uint32_t vmid,
			uint32_t vmhub, uint32_t flush_type)
{
	uint32_t inv_req, req, ack;
	const unsigned int eng = 17;
	struct amdgv_vmhub *hub;
	int ret = 0;

	hub = &adapt->vmhub[vmhub];
	if (!hub->vmhub_funcs || !hub->vmhub_funcs->get_invalidate_req)
		return AMDGV_FAILURE;

	inv_req = hub->vmhub_funcs->get_invalidate_req(vmid, flush_type);
	req = hub->vm_inv_eng0_req + hub->eng_distance * eng;
	ack = hub->vm_inv_eng0_ack + hub->eng_distance * eng;

	if (!req)
		return ret;

	WREG32(req, inv_req);
	AMDGV_DEBUG("req:%x, ack:%x, inv_req:%x\n", req, ack, inv_req);

	ret = amdgv_wait_for_register(
				adapt, ack, "VM_INVALIDATE_ENG_ACK",
				0xFFFFFFFF, 1 << vmid,
				AMDGV_TIMEOUT(TIMEOUT_STATUS_REG), AMDGV_WAIT_CHECK_EQ,
				AMDGV_WAIT_FLAG_AUTO);

	if ((vmhub != AMDGV_GFXHUB(0)) && (hub->vm_l2_bank_select_reserved_cid2)) {
		inv_req = RREG32(hub->vm_l2_bank_select_reserved_cid2);
		/* bit 25: RSERVED_CACHE_PRIVATE_INVALIDATION */
		inv_req |= (1 << 25);
		/* Issue private invalidation */
		WREG32(hub->vm_l2_bank_select_reserved_cid2, inv_req);
		/* Read back to ensure invalidation is done*/
		RREG32(hub->vm_l2_bank_select_reserved_cid2);
	}

	if (ret)
		AMDGV_ERROR("Timeout waiting for VM flush ACK!, ack:%x\n", RREG32(ack));

	return ret;
}

static uint64_t gmc_v11_0_get_gart_map_flags(struct amdgv_adapter *adapt)
{
	uint64_t flags = 0;

	flags = AMDGV_PTE_MTYPE_GFX11(MTYPE_UC);
	flags |= AMDGV_PTE_EXECUTABLE;
	flags |= AMDGV_PTE_VALID;
	flags |= AMDGV_PTE_READABLE;
	flags |= AMDGV_PTE_WRITEABLE;
	flags |= AMDGV_PTE_SNOOPED;
	flags |= AMDGV_PTE_SYSTEM;

	return flags;
}

static const struct amdgv_gmc_funcs gmc_v11_0_gmc_funcs = {
	.flush_gpu_tlb = gmc_v11_0_flush_gpu_tlb,
	.get_gart_map_flags = gmc_v11_0_get_gart_map_flags,
};

static void gmc_v11_0_set_gmc_funcs(struct amdgv_adapter *adapt)
{
	adapt->gmc.funcs = &gmc_v11_0_gmc_funcs;
}

static int navi32_mem_sw_init(struct amdgv_adapter *adapt)
{
	uint64_t libgv_res_fb_offset;
	uint64_t libgv_res_fb_size;
	uint32_t ptb_table_size;

	gfx_v11_gfxhub_init(adapt);
	mmhub_v3_0_init(adapt);
	gmc_v11_0_set_gmc_funcs(adapt);

	/* Use hypervisor's configuration to allocate FB for PF memmgr */
	if (adapt->opt.libgv_res_fb_size != AMDGV_USE_DEFAULT_MEMMGR) {
		libgv_res_fb_offset = adapt->opt.libgv_res_fb_offset;
		libgv_res_fb_size = adapt->opt.libgv_res_fb_size;
	} else {
		libgv_res_fb_offset = 0x0;
		libgv_res_fb_size = DEFAULT_MEMORY_MANAGER_SIZE;
	}

	if (amdgv_memmgr_init(adapt, &adapt->memmgr_pf, libgv_res_fb_offset, libgv_res_fb_size,
			      0, false)) {
		AMDGV_ERROR("Failed to init PF FB memory manager\n");
		goto pf_fail;
	}

	/* Allocate a memory manager for the whole GPU Framebuffer at top
	 * of memory. Allocate just below
	 */
	if (amdgv_memmgr_init(adapt, &adapt->memmgr_gpu, NAVI3_RESERVED_FB_SIZE,
			      DEFAULT_MEMORY_MANAGER_SIZE, 0, true)) {
		AMDGV_ERROR("Failed to init GPU FB memory manager\n");
		goto gpu_fail;
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
	amdgv_memmgr_fini(adapt, &adapt->memmgr_sys);
sys_fail:
	amdgv_wb_memory_fini(adapt);
wb_fail:
	amdgv_memmgr_fini(adapt, &adapt->memmgr_gpu);
gpu_fail:
	amdgv_memmgr_fini(adapt, &adapt->memmgr_pf);
pf_fail:
	return AMDGV_FAILURE;
}

static int navi32_mem_sw_fini(struct amdgv_adapter *adapt)
{
	amdgv_memmgr_free(adapt->pdb0_mem);
	amdgv_memmgr_free(adapt->ptb_mem);

	amdgv_wb_memory_fini(adapt);
	amdgv_memmgr_fini(adapt, &adapt->memmgr_sys);
	amdgv_memmgr_fini(adapt, &adapt->memmgr_pf);
	amdgv_memmgr_fini(adapt, &adapt->memmgr_gpu);

	return 0;
}

static int navi32_mem_gart_enable(struct amdgv_adapter *adapt)
{
	int ret = 0;

	AMDGV_INFO("pdb0: VA: %p PA: %llx BA: %llx\n", amdgv_memmgr_get_cpu_addr(adapt->pdb0_mem),
	    amdgv_memmgr_get_gpu_pa(adapt->pdb0_mem), amdgv_memmgr_get_gpu_addr(adapt->pdb0_mem));

	AMDGV_INFO("ptb: VA: %p PA: %llx BA: %llx\n", amdgv_memmgr_get_cpu_addr(adapt->ptb_mem),
	    amdgv_memmgr_get_gpu_pa(adapt->ptb_mem), amdgv_memmgr_get_gpu_addr(adapt->ptb_mem));

	amdgv_gart_init_pdb0(adapt);

	adapt->gart_ready = true;

	ret = amdgv_map_sys_mem_allocs(&adapt->memmgr_sys, AMDGV_MAP);

	return ret;
}

static int navi32_mem_hw_init(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	uint64_t mc_fb_loc_base, addr;
	int ret = 0;

	tmp = RREG32(SOC15_REG_OFFSET(MMHUB, 0, regMMMC_VM_FB_LOCATION_BASE));
	tmp = REG_GET_FIELD(tmp, MMMC_VM_FB_LOCATION_BASE, FB_BASE);

	mc_fb_loc_base = (uint64_t)tmp;
	mc_fb_loc_base = mc_fb_loc_base << MC_VM_FB_LOCATION__FB_ADDRESS__SHIFT;

	/* Initialize Manager at the base of FB */
	amdgv_memmgr_set_gpu_base(&adapt->memmgr_pf, mc_fb_loc_base);
	amdgv_memmgr_set_cpu_base(&adapt->memmgr_pf, adapt->fb);

	/* GPU manager at top of memory */
	addr = amdgv_misc_get_memsize(adapt);
	addr = (addr << 20);

	amdgv_memmgr_set_gpu_base(&adapt->memmgr_gpu, mc_fb_loc_base + addr);
	amdgv_memmgr_set_cpu_base(&adapt->memmgr_gpu,
				  (void *)(((uint32_t *)adapt->fb) + (addr >> 2)));

	ret = navi32_mem_gart_enable(adapt);
	if (ret)
		return ret;

	amdgv_wb_memory_hw_init_address(adapt);
	amdgv_wb_memory_clear(adapt);

	return ret;
}

static int navi32_mem_hw_fini(struct amdgv_adapter *adapt)
{
	amdgv_map_sys_mem_allocs(&adapt->memmgr_sys, AMDGV_UNMAP);
	adapt->gart_ready = false;

	return 0;
}

static int navi32_mem_hw_live_fini(struct amdgv_adapter *adapt)
{
	adapt->gart_ready = false;

	amdgv_map_sys_mem_allocs(&adapt->memmgr_sys, AMDGV_UNMAP);

	return 0;
}

const struct amdgv_init_func navi32_mem_func = {
	.name = "navi32_mem_func",
	.sw_init = navi32_mem_sw_init,
	.sw_fini = navi32_mem_sw_fini,
	.hw_init = navi32_mem_hw_init,
	.hw_fini = navi32_mem_hw_fini,
	.hw_live_init = navi32_mem_hw_init,
	.hw_live_fini = navi32_mem_hw_live_fini
};
