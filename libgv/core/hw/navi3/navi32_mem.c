/*
 * Copyright (C) 2021 Advanced Micro Devices, Inc. All rights reserved.
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
#include <amdgv_gpuiov.h>
#include <amdgv_memmgr.h>
#include <amdgv_wb_memory.h>

#include "navi32_reg_inc.h"
#include "navi32_ip_discovery.h"

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

static int navi32_mem_sw_init(struct amdgv_adapter *adapt)
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

	if (amdgv_memmgr_init(adapt, &adapt->memmgr_pf, libgv_res_fb_offset, libgv_res_fb_size,
			      0, false)) {
		AMDGV_ERROR("Failed to init PF FB memory manager\n");
		return AMDGV_FAILURE;
	}

	/* Allocate a memory manager for the whole GPU Framebuffer at top
	 * of memory. Allocate just below
	 */
	if (amdgv_memmgr_init(adapt, &adapt->memmgr_gpu, NAVI3_RESERVED_FB_SIZE,
			      DEFAULT_MEMORY_MANAGER_SIZE, 0, true)) {
		AMDGV_ERROR("Failed to init GPU FB memory manager\n");
		amdgv_memmgr_fini(adapt, &adapt->memmgr_pf);
		return AMDGV_FAILURE;
	}
	if (amdgv_wb_memory_init(adapt)) {
		AMDGV_ERROR("Failed to init GPU FB WB memory\n");
		amdgv_memmgr_fini(adapt, &adapt->memmgr_pf);
		amdgv_memmgr_fini(adapt, &adapt->memmgr_gpu);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int navi32_mem_sw_fini(struct amdgv_adapter *adapt)
{
	amdgv_wb_memory_fini(adapt);

	amdgv_memmgr_fini(adapt, &adapt->memmgr_pf);
	amdgv_memmgr_fini(adapt, &adapt->memmgr_gpu);

	return 0;
}

static int navi32_mem_hw_init(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	uint64_t mc_fb_loc_base, addr;

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

	amdgv_wb_memory_hw_init_address(adapt);
	amdgv_wb_memory_clear(adapt);

	return 0;
}

static int navi32_mem_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func navi32_mem_func = {
	.name = "navi32_mem_func",
	.sw_init = navi32_mem_sw_init,
	.sw_fini = navi32_mem_sw_fini,
	.hw_init = navi32_mem_hw_init,
	.hw_fini = navi32_mem_hw_fini,
};
