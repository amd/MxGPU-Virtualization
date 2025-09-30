/*
 * Copyright 2022 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <amdgv.h>
#include <amdgv_device.h>
#include <amdgv_gart.h>
#include "amdgv_mmhub.h"
#include "mi300/MMHUB/mmhub_1_8_0_offset.h"
#include "mi300/MMHUB/mmhub_1_8_0_sh_mask.h"
#include "mmhub_v1_8.h"
#include "mi300/HDP/hdp_4_4_2_offset.h"
#include "mi300/HDP/hdp_4_4_2_sh_mask.h"

#define regVM_L2_CNTL3_DEFAULT 0x80100007
#define regVM_L2_CNTL4_DEFAULT 0x000000c1

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static void mmhub_v1_8_init_gart_aperture_regs(struct amdgv_adapter *adapt)
{
	uint64_t page_table_base;
	int i;

	page_table_base = amdgv_memmgr_get_gpu_pa(adapt->pdb0_mem);
	page_table_base |= AMDGV_PTE_VALID;
	page_table_base |= AMDGV_PTE_SNOOPED;

	for (i = 0; i < adapt->mcp.num_aid; i++) {
		WREG32_SOC15(MMHUB, i, regVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32,
			     lower_32_bits(page_table_base));

		WREG32_SOC15(MMHUB, i, regVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_HI32,
			     upper_32_bits(page_table_base));

		WREG32_SOC15(MMHUB, i, regVM_CONTEXT0_PAGE_TABLE_START_ADDR_LO32,
			     (uint32_t)(GART_START >> 12));
		WREG32_SOC15(MMHUB, i, regVM_CONTEXT0_PAGE_TABLE_START_ADDR_HI32,
			     (uint32_t)(GART_START >> 44));

		WREG32_SOC15(MMHUB, i, regVM_CONTEXT0_PAGE_TABLE_END_ADDR_LO32,
			     (uint32_t)((adapt->gart_size + GART_START) >> 12));
		WREG32_SOC15(MMHUB, i, regVM_CONTEXT0_PAGE_TABLE_END_ADDR_HI32,
			     (uint32_t)((adapt->gart_size + GART_START) >> 44));
	}
}

static void mmhub_v1_8_init_system_aperture_regs(struct amdgv_adapter *adapt)
{
	uint64_t value;
	uint32_t tmp;
	uint32_t i = 0;
	uint32_t hdp_nonsurface_base_lo;
	uint32_t hdp_nonsurface_base_hi;

	hdp_nonsurface_base_lo = RREG32_SOC15(HDP, 0, regHDP_NONSURFACE_BASE);
	hdp_nonsurface_base_hi = RREG32_SOC15(HDP, 0, regHDP_NONSURFACE_BASE_HI);

	for (i = 0; i < adapt->mcp.num_aid; i++) {
		if (adapt->mc_sys_loc_addr && adapt->mc_sys_top_addr) {
			WREG32_SOC15(MMHUB, i, regMC_VM_SYSTEM_APERTURE_LOW_ADDR,
				     adapt->mc_sys_loc_addr >> 18);
			WREG32_SOC15(MMHUB, i, regMC_VM_SYSTEM_APERTURE_HIGH_ADDR,
				     adapt->mc_sys_top_addr >> 18);
		}

		/* Set MMMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR to HDP_NONSURFACE_BASE */
		WREG32_SOC15(MMHUB, i, regMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR_LSB, hdp_nonsurface_base_lo);
		WREG32_SOC15(MMHUB, i, regMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR_MSB, hdp_nonsurface_base_hi);

		/* Program "protection fault". */
		value = 0;
		WREG32_SOC15(MMHUB, i, regVM_L2_PROTECTION_FAULT_DEFAULT_ADDR_LO32,
			     (uint32_t)(value >> 12));
		WREG32_SOC15(MMHUB, i, regVM_L2_PROTECTION_FAULT_DEFAULT_ADDR_HI32,
			     (uint32_t)((uint64_t)value >> 44));

		tmp = RREG32_SOC15(MMHUB, i, regVM_L2_PROTECTION_FAULT_CNTL2);
		tmp = REG_SET_FIELD(tmp, VM_L2_PROTECTION_FAULT_CNTL2,
				    ACTIVE_PAGE_MIGRATION_PTE_READ_RETRY, 1);
		WREG32_SOC15(MMHUB, i, regVM_L2_PROTECTION_FAULT_CNTL2, tmp);
	}
}

static void mmhub_v1_8_init_tlb_regs(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	uint32_t i;

	for (i = 0; i < adapt->mcp.num_aid; i++) {
		/* Setup TLB control */
		tmp = RREG32_SOC15(MMHUB, i, regMC_VM_MX_L1_TLB_CNTL);

		tmp = REG_SET_FIELD(tmp, MC_VM_MX_L1_TLB_CNTL, ENABLE_L1_TLB, 1);
		tmp = REG_SET_FIELD(tmp, MC_VM_MX_L1_TLB_CNTL, SYSTEM_ACCESS_MODE, 3);
		tmp = REG_SET_FIELD(tmp, MC_VM_MX_L1_TLB_CNTL, ENABLE_ADVANCED_DRIVER_MODEL,
				    1);
		tmp = REG_SET_FIELD(tmp, MC_VM_MX_L1_TLB_CNTL, SYSTEM_APERTURE_UNMAPPED_ACCESS,
				    0);
		tmp = REG_SET_FIELD(tmp, MC_VM_MX_L1_TLB_CNTL, MTYPE, MTYPE_UC);
		tmp = REG_SET_FIELD(tmp, MC_VM_MX_L1_TLB_CNTL, ATC_EN, 1);

		WREG32_SOC15(MMHUB, i, regMC_VM_MX_L1_TLB_CNTL, tmp);
	}
}

static void mmhub_v1_8_init_cache_regs(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	uint32_t i = 0;

	for (i = 0; i < adapt->mcp.num_aid; i++) {
		/* Setup L2 cache */
		tmp = RREG32_SOC15(MMHUB, i, regVM_L2_CNTL);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL, ENABLE_L2_CACHE, 1);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL, ENABLE_L2_FRAGMENT_PROCESSING, 1);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL, L2_PDE0_CACHE_TAG_GENERATION_MODE, 0);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL, PDE_FAULT_CLASSIFICATION, 0);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL, CONTEXT1_IDENTITY_ACCESS_MODE, 1);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL, IDENTITY_MODE_FRAGMENT_SIZE, 0);
		WREG32_SOC15(MMHUB, i, regVM_L2_CNTL, tmp);

		tmp = RREG32_SOC15(MMHUB, i, regVM_L2_CNTL2);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL2, INVALIDATE_ALL_L1_TLBS, 1);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL2, INVALIDATE_L2_CACHE, 1);
		WREG32_SOC15(MMHUB, i, regVM_L2_CNTL2, tmp);

		tmp = regVM_L2_CNTL3_DEFAULT;
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL3, BANK_SELECT, 12);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL3, L2_CACHE_BIGK_FRAGMENT_SIZE, 9);
		WREG32_SOC15(MMHUB, i, regVM_L2_CNTL3, tmp);

		tmp = regVM_L2_CNTL4_DEFAULT;
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL4, VMC_TAP_PDE_REQUEST_PHYSICAL, 0);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL4, VMC_TAP_PTE_REQUEST_PHYSICAL, 0);
		WREG32_SOC15(MMHUB, i, regVM_L2_CNTL4, tmp);
	}
}

/* Set snoop bit for SDMA so that SDMA writes probe-invalidates RW lines */
static void mmhub_v1_8_init_snoop_override_regs(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	int i, j;
	uint32_t distance = regDAGB1_WRCLI_GPU_SNOOP_OVERRIDE -
			    regDAGB0_WRCLI_GPU_SNOOP_OVERRIDE;

	for (i = 0; i < adapt->mcp.num_aid; i++) {
		for (j = 0; j < 5; j++) { /* DAGB instances */
			tmp = RREG32_SOC15_OFFSET(MMHUB, i,
				regDAGB0_WRCLI_GPU_SNOOP_OVERRIDE, j * distance);
			tmp |= (1 << 15); /* SDMA client is BIT15 */
			WREG32_SOC15_OFFSET(MMHUB, i,
				regDAGB0_WRCLI_GPU_SNOOP_OVERRIDE, j * distance, tmp);

			tmp = RREG32_SOC15_OFFSET(MMHUB, i,
				regDAGB0_WRCLI_GPU_SNOOP_OVERRIDE_VALUE, j * distance);
			tmp |= (1 << 15);
			WREG32_SOC15_OFFSET(MMHUB, i,
				regDAGB0_WRCLI_GPU_SNOOP_OVERRIDE_VALUE, j * distance, tmp);
		}
	}
}

static void mmhub_v1_8_enable_system_domain(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	int i, j;
	struct amdgv_vmhub *hub;

	for (i = 0; i < adapt->mcp.num_aid; i++) {
		tmp = RREG32_SOC15(MMHUB, i, regVM_CONTEXT0_CNTL);
		tmp = REG_SET_FIELD(tmp, VM_CONTEXT0_CNTL, ENABLE_CONTEXT, 1);
		tmp = REG_SET_FIELD(tmp, VM_CONTEXT0_CNTL, PAGE_TABLE_DEPTH, 1);
		tmp = REG_SET_FIELD(tmp, VM_CONTEXT0_CNTL, PAGE_TABLE_BLOCK_SIZE, 12);
		tmp = REG_SET_FIELD(tmp, VM_CONTEXT0_CNTL,
				    RETRY_PERMISSION_OR_INVALID_PAGE_FAULT, 0);
		WREG32_SOC15(MMHUB, i, regVM_CONTEXT0_CNTL, tmp);

		hub = &adapt->vmhub[AMDGV_MMHUB0(i)];
		if (!hub->eng_addr_distance)
			continue;

		for (j = 0; j < 18; ++j) {
			WREG32_SOC15_OFFSET(MMHUB, i,
					regVM_INVALIDATE_ENG0_ADDR_RANGE_LO32,
					j * hub->eng_addr_distance, 0xffffffff);
			WREG32_SOC15_OFFSET(MMHUB, i,
					regVM_INVALIDATE_ENG0_ADDR_RANGE_HI32,
					j * hub->eng_addr_distance, 0x1f);
		}
	}
}

static void mmhub_v1_8_disable_identity_aperture(struct amdgv_adapter *adapt)
{
	uint32_t i = 0;

	for (i = 0; i < adapt->mcp.num_aid; i++) {
		WREG32_SOC15(MMHUB, i, regVM_L2_CONTEXT1_IDENTITY_APERTURE_LOW_ADDR_LO32,
			     0XFFFFFFFF);
		WREG32_SOC15(MMHUB, i, regVM_L2_CONTEXT1_IDENTITY_APERTURE_LOW_ADDR_HI32,
			     0x0000000F);

		WREG32_SOC15(MMHUB, i, regVM_L2_CONTEXT1_IDENTITY_APERTURE_HIGH_ADDR_LO32, 0);
		WREG32_SOC15(MMHUB, i, regVM_L2_CONTEXT1_IDENTITY_APERTURE_HIGH_ADDR_HI32, 0);

		WREG32_SOC15(MMHUB, i, regVM_L2_CONTEXT_IDENTITY_PHYSICAL_OFFSET_LO32, 0);
		WREG32_SOC15(MMHUB, i, regVM_L2_CONTEXT_IDENTITY_PHYSICAL_OFFSET_HI32, 0);
	}
}

static void mmhub_v1_8_set_fault_enable_default(struct amdgv_adapter *adapt, bool value)
{
	uint32_t tmp;
	uint32_t i = 0;

	for (i = 0; i < adapt->mcp.num_aid; i++) {
		tmp = RREG32_SOC15(MMHUB, i, regVM_L2_PROTECTION_FAULT_CNTL);
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

		WREG32_SOC15(MMHUB, i, regVM_L2_PROTECTION_FAULT_CNTL, tmp);
	}
}

void mmhub_v1_8_enable_xgmi(struct amdgv_adapter *adapt)
{
	uint32_t xgmi_enable;
	uint32_t i;

	xgmi_enable = 0xFFFF | (1 << 31); /* enable all VFs and PF for xgmi */

	for (i = 0; i < adapt->mcp.num_aid; i++) {
		WREG32_SOC15(MMHUB, i, regMC_VM_XGMI_GPUIOV_ENABLE, xgmi_enable);
		xgmi_enable = RREG32_SOC15(MMHUB, i, regMC_VM_XGMI_GPUIOV_ENABLE);
	}
}

void mmhub_v1_8_gart_enable(struct amdgv_adapter *adapt)
{
	mmhub_v1_8_init_gart_aperture_regs(adapt);
	mmhub_v1_8_init_system_aperture_regs(adapt);
	mmhub_v1_8_init_tlb_regs(adapt);
	mmhub_v1_8_init_cache_regs(adapt);
	mmhub_v1_8_init_snoop_override_regs(adapt);
	mmhub_v1_8_enable_system_domain(adapt);
	mmhub_v1_8_disable_identity_aperture(adapt);
	mmhub_v1_8_set_fault_enable_default(adapt, true);
}

void mmhub_v1_8_init(struct amdgv_adapter *adapt)
{
	struct amdgv_vmhub *hub;
	int i;

	for (i = 0; i < adapt->mcp.num_aid; i++) {
		hub = &adapt->vmhub[AMDGV_MMHUB0(i)];

		hub->ctx0_ptb_addr_lo32 = SOC15_REG_OFFSET(MMHUB, i,
			regVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32);
		hub->ctx0_ptb_addr_hi32 = SOC15_REG_OFFSET(MMHUB, i,
			regVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_HI32);
		hub->vm_inv_eng0_req =
			SOC15_REG_OFFSET(MMHUB, i, regVM_INVALIDATE_ENG0_REQ);
		hub->vm_inv_eng0_ack =
			SOC15_REG_OFFSET(MMHUB, i, regVM_INVALIDATE_ENG0_ACK);
		hub->vm_context0_cntl =
			SOC15_REG_OFFSET(MMHUB, i, regVM_CONTEXT0_CNTL);
		hub->vm_l2_pro_fault_status = SOC15_REG_OFFSET(MMHUB, i,
			regVM_L2_PROTECTION_FAULT_STATUS);
		hub->vm_l2_pro_fault_cntl = SOC15_REG_OFFSET(MMHUB, i,
			regVM_L2_PROTECTION_FAULT_CNTL);

		hub->ctx_distance = regVM_CONTEXT1_CNTL - regVM_CONTEXT0_CNTL;
		hub->ctx_addr_distance =
			regVM_CONTEXT1_PAGE_TABLE_BASE_ADDR_LO32 -
			regVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32;
		hub->eng_distance = regVM_INVALIDATE_ENG1_REQ -
			regVM_INVALIDATE_ENG0_REQ;
		hub->eng_addr_distance = regVM_INVALIDATE_ENG1_ADDR_RANGE_LO32 -
			regVM_INVALIDATE_ENG0_ADDR_RANGE_LO32;
	}
}

void mmhub_v1_8_gart_fini(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	uint32_t i = 0;

	for (i = 0; i < adapt->mcp.num_aid; i++) {
		/* Setup L2 cache */
		tmp = RREG32_SOC15(MMHUB, i, regVM_L2_CNTL);
		tmp = REG_SET_FIELD(tmp, VM_L2_CNTL, ENABLE_L2_CACHE, 0);
		WREG32_SOC15(MMHUB, i, regVM_L2_CNTL, tmp);
		WREG32_SOC15(MMHUB, i, regVM_L2_CNTL3, 0);
	}
}

static void mmhub_v1_8_query_ras_error_count(struct amdgv_adapter *adapt,
					     void *ras_err_status)
{
	adapt->mca.funcs->pop_block_error_count(adapt,
						AMDGV_RAS_BLOCK__MMHUB,
						ras_err_status);
}

void mmhub_v1_8_dirtybit_control(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t mm_value;
	int i;

	for (i = 0; i < adapt->mcp.num_aid; i++) {
		mm_value = RREG32(SOC15_REG_OFFSET(MMHUB, i, regMMEA0_MAM_CTRL));
		mm_value = REG_SET_FIELD(mm_value, MMEA0_MAM_CTRL, MAM_DISABLE, !enable);
		mm_value = REG_SET_FIELD(mm_value, MMEA0_MAM_CTRL, ADRAM_MODE, adapt->dirtybit.mam_adram_mode);
		WREG32(SOC15_REG_OFFSET(MMHUB, i, regMMEA0_MAM_CTRL), mm_value);
		mm_value = RREG32(SOC15_REG_OFFSET(MMHUB, i, regMMEA0_MAM_CTRL));
		AMDGV_DEBUG("AID(%d) regMMEA0_MAM_CTRL = 0x%x\n", i, mm_value);

		mm_value = RREG32(SOC15_REG_OFFSET(MMHUB, i, regMMEA1_MAM_CTRL));
		mm_value = REG_SET_FIELD(mm_value, MMEA1_MAM_CTRL, MAM_DISABLE, !enable);
		mm_value = REG_SET_FIELD(mm_value, MMEA1_MAM_CTRL, ADRAM_MODE, adapt->dirtybit.mam_adram_mode);
		WREG32(SOC15_REG_OFFSET(MMHUB, i, regMMEA1_MAM_CTRL), mm_value);
		mm_value = RREG32(SOC15_REG_OFFSET(MMHUB, i, regMMEA1_MAM_CTRL));
		AMDGV_DEBUG("AID(%d) regMMEA1_MAM_CTRL = 0x%x\n", i, mm_value);

		mm_value = RREG32(SOC15_REG_OFFSET(MMHUB, i, regMMEA2_MAM_CTRL));
		mm_value = REG_SET_FIELD(mm_value, MMEA2_MAM_CTRL, MAM_DISABLE, !enable);
		mm_value = REG_SET_FIELD(mm_value, MMEA2_MAM_CTRL, ADRAM_MODE, adapt->dirtybit.mam_adram_mode);
		WREG32(SOC15_REG_OFFSET(MMHUB, i, regMMEA2_MAM_CTRL), mm_value);
		mm_value = RREG32(SOC15_REG_OFFSET(MMHUB, i, regMMEA2_MAM_CTRL));
		AMDGV_DEBUG("AID(%d) regMMEA2_MAM_CTRL = 0x%x\n", i, mm_value);

		mm_value = RREG32(SOC15_REG_OFFSET(MMHUB, i, regMMEA3_MAM_CTRL));
		mm_value = REG_SET_FIELD(mm_value, MMEA3_MAM_CTRL, MAM_DISABLE, !enable);
		mm_value = REG_SET_FIELD(mm_value, MMEA3_MAM_CTRL, ADRAM_MODE, adapt->dirtybit.mam_adram_mode);
		WREG32(SOC15_REG_OFFSET(MMHUB, i, regMMEA3_MAM_CTRL), mm_value);
		mm_value = RREG32(SOC15_REG_OFFSET(MMHUB, i, regMMEA3_MAM_CTRL));
		AMDGV_DEBUG("AID(%d) regMMEA3_MAM_CTRL = 0x%x\n", i, mm_value);

		mm_value = RREG32(SOC15_REG_OFFSET(MMHUB, i, regMMEA4_MAM_CTRL));
		mm_value = REG_SET_FIELD(mm_value, MMEA4_MAM_CTRL, MAM_DISABLE, !enable);
		mm_value = REG_SET_FIELD(mm_value, MMEA4_MAM_CTRL, ADRAM_MODE, adapt->dirtybit.mam_adram_mode);
		WREG32(SOC15_REG_OFFSET(MMHUB, i, regMMEA4_MAM_CTRL), mm_value);
		mm_value = RREG32(SOC15_REG_OFFSET(MMHUB, i, regMMEA4_MAM_CTRL));
		AMDGV_DEBUG("AID(%d) regMMEA4_MAM_CTRL = 0x%x\n", i, mm_value);
	}
}

static const struct amdgv_mmhub_funcs mmhub_v1_8_funcs = {
	.err_cnt_init = NULL,
	.reset_ras_error_count = NULL,
	.query_ras_error_count = mmhub_v1_8_query_ras_error_count,
};

void mmhub_v1_8_set_ras_funcs(struct amdgv_adapter *adapt)
{
	AMDGV_INFO("MMHUB: num_instances:%d, active_mask:0x%llx\n", adapt->mmhub.num_instances, adapt->mmhub.active_mask);
	adapt->mmhub.funcs = &mmhub_v1_8_funcs;
}
