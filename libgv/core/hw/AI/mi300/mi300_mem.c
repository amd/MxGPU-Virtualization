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

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

/* Default size is 256MB for the manager */
#define DEFAULT_MEMORY_MANAGER_SIZE (256 << 20)

static int mi300_mem_sw_init(struct amdgv_adapter *adapt)
{
	uint64_t libgv_res_fb_offset;
	uint64_t libgv_res_fb_size;

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
		return AMDGV_FAILURE;
	} else if (amdgv_wb_memory_init(adapt)) {
		AMDGV_ERROR("Failed to init GPU FB WB memory\n");
		amdgv_memmgr_fini(adapt, &adapt->memmgr_pf);
		return AMDGV_FAILURE;
	}

	if (amdgv_memmgr_init(adapt, &adapt->memmgr_gpu, 0, DEFAULT_MEMORY_MANAGER_SIZE, 0, true)) {
			AMDGV_ERROR("Failed to init GPU FB memory manager\n");
			amdgv_memmgr_fini(adapt, &adapt->memmgr_pf);
			amdgv_wb_memory_fini(adapt);
			return AMDGV_FAILURE;
	}

	return 0;
}

static int mi300_mem_sw_fini(struct amdgv_adapter *adapt)
{
	amdgv_wb_memory_fini(adapt);
	amdgv_memmgr_fini(adapt, &adapt->memmgr_pf);
	amdgv_memmgr_fini(adapt, &adapt->memmgr_gpu);

	return 0;
}

static int mi300_mem_hw_init(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	uint64_t mc_fb_loc_base;
	uint64_t offset;

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

	amdgv_wb_memory_hw_init_address(adapt);
	amdgv_wb_memory_clear(adapt);

	return 0;
}

static int mi300_mem_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func mi300_mem_func = {
	.name = "mi300_mem_func",
	.sw_init = mi300_mem_sw_init,
	.sw_fini = mi300_mem_sw_fini,
	.hw_init = mi300_mem_hw_init,
	.hw_fini = mi300_mem_hw_fini,
};
