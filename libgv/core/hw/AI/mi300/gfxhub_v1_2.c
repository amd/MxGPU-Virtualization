/*
 * Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
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

#include "gfxhub_v1_2.h"
#include "mi300/GC/gc_9_4_3_offset.h"
#include "mi300/GC/gc_9_4_3_sh_mask.h"
#include "mi300/HDP/hdp_4_4_2_offset.h"
#include "mi300/HDP/hdp_4_4_2_sh_mask.h"

#define regVM_L2_CNTL3_DEFAULT 0x80100007
#define regVM_L2_CNTL4_DEFAULT 0x000000c1

uint64_t gfxhub_v1_2_get_mc_fb_offset(struct amdgv_adapter *adapt)
{
	return (uint64_t)RREG32_SOC15(GC, GET_INST(GC, 0), regMC_VM_FB_OFFSET) << 24;
}

static void gfxhub_v1_2_init_system_aperture_regs(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	uint32_t hdp_nonsurface_base_lo;
	uint32_t hdp_nonsurface_base_hi;
	int i;

	hdp_nonsurface_base_lo = RREG32_SOC15(HDP, 0, regHDP_NONSURFACE_BASE);
	hdp_nonsurface_base_hi = RREG32_SOC15(HDP, 0, regHDP_NONSURFACE_BASE_HI);

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
		if (adapt->mc_sys_loc_addr && adapt->mc_sys_top_addr) {
			WREG32_SOC15(GC, GET_INST(GC, i),
				     regMC_VM_SYSTEM_APERTURE_LOW_ADDR,
				     adapt->mc_sys_loc_addr >> 18);
			WREG32_SOC15(GC, GET_INST(GC, i),
				     regMC_VM_SYSTEM_APERTURE_HIGH_ADDR,
				     adapt->mc_sys_top_addr >> 18);
		}

		/* Set GCMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR to HDP_NONSURFACE_BASE(FB start) */
		WREG32_SOC15(GC, GET_INST(GC, i), regMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR_LSB, hdp_nonsurface_base_lo);
		WREG32_SOC15(GC, GET_INST(GC, i), regMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR_MSB, hdp_nonsurface_base_hi);

		/* Set default protection fault address */
		WREG32_SOC15(GC, GET_INST(GC, i), regVM_L2_PROTECTION_FAULT_DEFAULT_ADDR_LO32,
			     0);
		WREG32_SOC15(GC, GET_INST(GC, i), regVM_L2_PROTECTION_FAULT_DEFAULT_ADDR_HI32,
			     0);

		tmp = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, i),
					      regVM_L2_PROTECTION_FAULT_CNTL2));
		tmp = REG_SET_FIELD(tmp, VM_L2_PROTECTION_FAULT_CNTL2,
				    ACTIVE_PAGE_MIGRATION_PTE_READ_RETRY, 1);
		WREG32_SOC15(GC, GET_INST(GC, i), regVM_L2_PROTECTION_FAULT_CNTL2, tmp);
	}
}

static void gfxhub_v1_2_init_tlb_regs(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	int i;

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
		/* Setup TLB control */
		tmp = RREG32_SOC15(GC, GET_INST(GC, i), regMC_VM_MX_L1_TLB_CNTL);

		tmp = REG_SET_FIELD(tmp, MC_VM_MX_L1_TLB_CNTL, ENABLE_L1_TLB, 1);
		tmp = REG_SET_FIELD(tmp, MC_VM_MX_L1_TLB_CNTL, SYSTEM_ACCESS_MODE, 3);
		tmp = REG_SET_FIELD(tmp, MC_VM_MX_L1_TLB_CNTL, ENABLE_ADVANCED_DRIVER_MODEL,
				    1);
		tmp = REG_SET_FIELD(tmp, MC_VM_MX_L1_TLB_CNTL, SYSTEM_APERTURE_UNMAPPED_ACCESS,
				    0);
		tmp = REG_SET_FIELD(tmp, MC_VM_MX_L1_TLB_CNTL, MTYPE, MTYPE_UC);
		tmp = REG_SET_FIELD(tmp, MC_VM_MX_L1_TLB_CNTL, ATC_EN, 1);

		WREG32_SOC15(GC, GET_INST(GC, i), regMC_VM_MX_L1_TLB_CNTL, tmp);
	}
}

static void gfxhub_v1_2_init_cache_regs(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	int i;

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
		/* Setup L2 cache */
		tmp = RREG32_SOC15(GC, GET_INST(GC, i), regVM_L2_CNTL);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL, ENABLE_L2_CACHE, 1);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL, ENABLE_L2_FRAGMENT_PROCESSING, 1);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL, L2_PDE0_CACHE_TAG_GENERATION_MODE, 0);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL, PDE_FAULT_CLASSIFICATION, 0);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL, CONTEXT1_IDENTITY_ACCESS_MODE, 1);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL, IDENTITY_MODE_FRAGMENT_SIZE, 0);
		WREG32_SOC15(GC, GET_INST(GC, i), regVM_L2_CNTL, tmp);

		tmp = RREG32_SOC15(GC, GET_INST(GC, i), regVM_L2_CNTL2);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL2, INVALIDATE_ALL_L1_TLBS, 1);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL2, INVALIDATE_L2_CACHE, 1);
		WREG32_SOC15(GC, GET_INST(GC, i), regVM_L2_CNTL2, tmp);

		tmp = regVM_L2_CNTL3_DEFAULT;
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL3, BANK_SELECT, 12);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL3, L2_CACHE_BIGK_FRAGMENT_SIZE, 9);
		WREG32_SOC15(GC, GET_INST(GC, i), regVM_L2_CNTL3, tmp);

		tmp = regVM_L2_CNTL4_DEFAULT;
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL4, VMC_TAP_PDE_REQUEST_PHYSICAL, 0);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL4, VMC_TAP_PTE_REQUEST_PHYSICAL, 0);
		WREG32_SOC15(GC, GET_INST(GC, i), regVM_L2_CNTL4, tmp);
	}
}

static void gfxhub_v1_2_disable_identity_aperture(struct amdgv_adapter *adapt)
{
	int i;

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
		WREG32_SOC15(GC, GET_INST(GC, i),
			     regVM_L2_CONTEXT1_IDENTITY_APERTURE_LOW_ADDR_LO32, 0XFFFFFFFF);
		WREG32_SOC15(GC, GET_INST(GC, i),
			     regVM_L2_CONTEXT1_IDENTITY_APERTURE_LOW_ADDR_HI32, 0x0000000F);

		WREG32_SOC15(GC, GET_INST(GC, i),
			     regVM_L2_CONTEXT1_IDENTITY_APERTURE_HIGH_ADDR_LO32, 0);
		WREG32_SOC15(GC, GET_INST(GC, i),
			     regVM_L2_CONTEXT1_IDENTITY_APERTURE_HIGH_ADDR_HI32, 0);

		WREG32_SOC15(GC, GET_INST(GC, i),
			     regVM_L2_CONTEXT_IDENTITY_PHYSICAL_OFFSET_LO32, 0);
		WREG32_SOC15(GC, GET_INST(GC, i),
			     regVM_L2_CONTEXT_IDENTITY_PHYSICAL_OFFSET_HI32, 0);
	}
}

static void gfxhub_v1_2_set_fault_enable_default(struct amdgv_adapter *adapt, bool value)
{
	uint32_t tmp;
	uint32_t i = 0;

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
		/* regVM_L2_PROTECTION_FAULT_CNTL */
		tmp = RREG32(
			SOC15_REG_OFFSET(GC, GET_INST(GC, i), regVM_L2_PROTECTION_FAULT_CNTL));
		tmp = REG_SET_FIELD(tmp, VM_L2_PROTECTION_FAULT_CNTL,
				    RANGE_PROTECTION_FAULT_ENABLE_DEFAULT, value);
		tmp = REG_SET_FIELD(tmp, VM_L2_PROTECTION_FAULT_CNTL,
				    PDE0_PROTECTION_FAULT_ENABLE_DEFAULT, value);
		tmp = REG_SET_FIELD(tmp, VM_L2_PROTECTION_FAULT_CNTL,
				    PDE1_PROTECTION_FAULT_ENABLE_DEFAULT, value);
		tmp = REG_SET_FIELD(tmp, VM_L2_PROTECTION_FAULT_CNTL,
				    PDE2_PROTECTION_FAULT_ENABLE_DEFAULT, value);
		tmp = REG_SET_FIELD(tmp, VM_L2_PROTECTION_FAULT_CNTL,
				    TRANSLATE_FURTHER_PROTECTION_FAULT_ENABLE_DEFAULT, value);
		tmp = REG_SET_FIELD(tmp, VM_L2_PROTECTION_FAULT_CNTL,
				    NACK_PROTECTION_FAULT_ENABLE_DEFAULT, value);
		tmp = REG_SET_FIELD(tmp, VM_L2_PROTECTION_FAULT_CNTL,
				    DUMMY_PAGE_PROTECTION_FAULT_ENABLE_DEFAULT, value);
		tmp = REG_SET_FIELD(tmp, VM_L2_PROTECTION_FAULT_CNTL,
				    VALID_PROTECTION_FAULT_ENABLE_DEFAULT, value);
		tmp = REG_SET_FIELD(tmp, VM_L2_PROTECTION_FAULT_CNTL,
				    READ_PROTECTION_FAULT_ENABLE_DEFAULT, value);
		tmp = REG_SET_FIELD(tmp, VM_L2_PROTECTION_FAULT_CNTL,
				    WRITE_PROTECTION_FAULT_ENABLE_DEFAULT, value);
		tmp = REG_SET_FIELD(tmp, VM_L2_PROTECTION_FAULT_CNTL,
				    EXECUTE_PROTECTION_FAULT_ENABLE_DEFAULT, value);
		if (!value) {
			tmp = REG_SET_FIELD(tmp, VM_L2_PROTECTION_FAULT_CNTL,
					    CRASH_ON_NO_RETRY_FAULT, 1);
			tmp = REG_SET_FIELD(tmp, VM_L2_PROTECTION_FAULT_CNTL,
					    CRASH_ON_RETRY_FAULT, 1);
		}
		WREG32_SOC15(GC, GET_INST(GC, i), regVM_L2_PROTECTION_FAULT_CNTL, tmp);
	}
}

void gfxhub_v1_2_vmhub_hook(struct amdgv_adapter *adapt)
{
	adapt->vmhub[VM_GFXHUB].fault_cntl =
		SOC15_REG_OFFSET(GC, 0, regVM_L2_PROTECTION_FAULT_CNTL);
	adapt->vmhub[VM_GFXHUB].fault_status =
		SOC15_REG_OFFSET(GC, 0, regVM_L2_PROTECTION_FAULT_STATUS);
}

void gfxhub_v1_2_enable_system_context(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	uint32_t i = 0;

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
		/* config context0 to trap all kinds of page fault */
		tmp = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, i), regVM_CONTEXT0_CNTL));
		tmp = REG_SET_FIELD(tmp, VM_CONTEXT0_CNTL, ENABLE_CONTEXT, 1);
		tmp = REG_SET_FIELD(tmp, VM_CONTEXT0_CNTL, PAGE_TABLE_DEPTH, 0);
		tmp = REG_SET_FIELD(tmp, VM_CONTEXT0_CNTL,
				    RETRY_PERMISSION_OR_INVALID_PAGE_FAULT, 0);
		tmp = REG_SET_FIELD(tmp, VM_CONTEXT0_CNTL,
				    RANGE_PROTECTION_FAULT_ENABLE_INTERRUPT, 1);
		tmp = REG_SET_FIELD(tmp, VM_CONTEXT0_CNTL,
				    DUMMY_PAGE_PROTECTION_FAULT_ENABLE_INTERRUPT, 1);
		tmp = REG_SET_FIELD(tmp, VM_CONTEXT0_CNTL,
				    PDE0_PROTECTION_FAULT_ENABLE_INTERRUPT, 1);
		tmp = REG_SET_FIELD(tmp, VM_CONTEXT0_CNTL,
				    VALID_PROTECTION_FAULT_ENABLE_INTERRUPT, 1);
		tmp = REG_SET_FIELD(tmp, VM_CONTEXT0_CNTL,
				    READ_PROTECTION_FAULT_ENABLE_INTERRUPT, 1);
		tmp = REG_SET_FIELD(tmp, VM_CONTEXT0_CNTL,
				    WRITE_PROTECTION_FAULT_ENABLE_INTERRUPT, 1);
		tmp = REG_SET_FIELD(tmp, VM_CONTEXT0_CNTL,
				    EXECUTE_PROTECTION_FAULT_ENABLE_INTERRUPT, 1);
		/* set GART logic space range from ~0 to 0 thus force all GART range page
		 * fault */
		WREG32_SOC15(GC, GET_INST(GC, i), regVM_CONTEXT0_CNTL, tmp);
		WREG32_SOC15(GC, GET_INST(GC, i), regVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32,
			     ~0);
		WREG32_SOC15(GC, GET_INST(GC, i), regVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_HI32,
			     ~0);
		WREG32_SOC15(GC, GET_INST(GC, i), regVM_CONTEXT0_PAGE_TABLE_START_ADDR_LO32,
			     ~0);
		WREG32_SOC15(GC, GET_INST(GC, i), regVM_CONTEXT0_PAGE_TABLE_START_ADDR_HI32,
			     ~0);
		WREG32_SOC15(GC, GET_INST(GC, i), regVM_CONTEXT0_PAGE_TABLE_END_ADDR_LO32, 0);
		WREG32_SOC15(GC, GET_INST(GC, i), regVM_CONTEXT0_PAGE_TABLE_END_ADDR_HI32, 0);

		/* Setup TLB control */
		tmp = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, i), regMC_VM_MX_L1_TLB_CNTL));
		tmp = REG_SET_FIELD(tmp, MC_VM_MX_L1_TLB_CNTL, ENABLE_L1_TLB, 1);
		tmp = REG_SET_FIELD(tmp, MC_VM_MX_L1_TLB_CNTL, SYSTEM_ACCESS_MODE, 3);
		tmp = REG_SET_FIELD(tmp, MC_VM_MX_L1_TLB_CNTL, ENABLE_ADVANCED_DRIVER_MODEL,
				    1);
		tmp = REG_SET_FIELD(tmp, MC_VM_MX_L1_TLB_CNTL, SYSTEM_APERTURE_UNMAPPED_ACCESS,
				    0);
		tmp = REG_SET_FIELD(tmp, MC_VM_MX_L1_TLB_CNTL, ECO_BITS, 0);
		tmp = REG_SET_FIELD(tmp, MC_VM_MX_L1_TLB_CNTL, MTYPE, 3); /* XXX for emulation.
									   */
		WREG32_SOC15(GC, GET_INST(GC, i), regMC_VM_MX_L1_TLB_CNTL, tmp);
	}
}

void gfxhub_v1_2_enable_xgmi(struct amdgv_adapter *adapt)
{
	uint32_t xgmi_enable;
	int i;

	xgmi_enable = 0xFFFF | (1 << 31); /* enable all VFs and PF for xgmi */
	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
		WREG32_SOC15(GC, GET_INST(GC, i), regMC_VM_XGMI_GPUIOV_ENABLE, xgmi_enable);
		xgmi_enable = RREG32_SOC15(GC, GET_INST(GC, i), regMC_VM_XGMI_GPUIOV_ENABLE);
	}
}

void gfxhub_v1_2_gart_enable(struct amdgv_adapter *adapt)
{
	gfxhub_v1_2_init_system_aperture_regs(adapt);
	gfxhub_v1_2_init_tlb_regs(adapt);
	gfxhub_v1_2_init_cache_regs(adapt);
	gfxhub_v1_2_disable_identity_aperture(adapt);
	gfxhub_v1_2_set_fault_enable_default(adapt, !is_debug_mode_hang());
}

void gfxhub_v1_2_gart_fini(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	uint32_t i = 0;

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
		/* regVM_L2_CNTL */
		tmp = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, i), regVM_L2_CNTL));
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL, ENABLE_L2_CACHE, 0);
		WREG32_SOC15(GC, GET_INST(GC, i), regVM_L2_CNTL, tmp);

		/* regVM_L2_CNTL3 */
		WREG32_SOC15(GC, GET_INST(GC, i), regVM_L2_CNTL3, 0);
	}
}
