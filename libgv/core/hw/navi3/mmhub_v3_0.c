/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv.h>
#include <amdgv_device.h>
#include <amdgv_gart.h>

#include <navi3/MMHUB/mmhub_3_0_0_offset.h>
#include <navi3/MMHUB/mmhub_3_0_0_sh_mask.h>
#include <navi3/HDP/hdp_6_0_0_offset.h>
#include <navi3/HDP/hdp_6_0_0_sh_mask.h>

#include "mmhub_v3_0.h"
#include "amdgv_mmhub.h"

#define regMMVM_L2_CNTL3_DEFAULT                0x80100007
#define regMMVM_L2_CNTL4_DEFAULT                0x000000c1
#define regMMVM_L2_CNTL5_DEFAULT                0x00003fe0

static void mmhub_v3_0_init_gart_aperture_regs(struct amdgv_adapter *adapt)
{
	uint64_t page_table_base;

	page_table_base = amdgv_memmgr_get_gpu_pa(adapt->pdb0_mem);
	page_table_base |= AMDGV_PTE_VALID;
	page_table_base |= AMDGV_PTE_SNOOPED;

	WREG32_SOC15(MMHUB, 0, regMMVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32,
			lower_32_bits(page_table_base));

	WREG32_SOC15(MMHUB, 0, regMMVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_HI32,
			upper_32_bits(page_table_base));

	WREG32_SOC15(MMHUB, 0, regMMVM_CONTEXT0_PAGE_TABLE_START_ADDR_LO32,
			(uint32_t)(GART_START >> 12));
	WREG32_SOC15(MMHUB, 0, regMMVM_CONTEXT0_PAGE_TABLE_START_ADDR_HI32,
			(uint32_t)(GART_START >> 44));

	WREG32_SOC15(MMHUB, 0, regMMVM_CONTEXT0_PAGE_TABLE_END_ADDR_LO32,
			(uint32_t)((adapt->gart_size + GART_START) >> 12));
	WREG32_SOC15(MMHUB, 0, regMMVM_CONTEXT0_PAGE_TABLE_END_ADDR_HI32,
			(uint32_t)((adapt->gart_size + GART_START) >> 44));
}

static void mmhub_v3_0_init_system_aperture_regs(struct amdgv_adapter *adapt)
{
	uint64_t value;
	uint32_t tmp;
	uint32_t hdp_nonsurface_base_lo;
	uint32_t hdp_nonsurface_base_hi;

	hdp_nonsurface_base_lo = RREG32_SOC15(HDP, 0, regHDP_NONSURFACE_BASE);
	hdp_nonsurface_base_hi = RREG32_SOC15(HDP, 0, regHDP_NONSURFACE_BASE_HI);

	if (adapt->mc_sys_loc_addr && adapt->mc_sys_top_addr) {
		WREG32_SOC15(MMHUB, 0, regMMMC_VM_SYSTEM_APERTURE_LOW_ADDR,
				adapt->mc_sys_loc_addr >> 18);
		WREG32_SOC15(MMHUB, 0, regMMMC_VM_SYSTEM_APERTURE_HIGH_ADDR,
				adapt->mc_sys_top_addr >> 18);
	}

	/* Set MMMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR to HDP_NONSURFACE_BASE */
	WREG32_SOC15(MMHUB, 0, regMMMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR_LSB, hdp_nonsurface_base_lo);
	WREG32_SOC15(MMHUB, 0, regMMMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR_MSB, hdp_nonsurface_base_hi);

	/* Program "protection fault". */
	value = 0;
	WREG32_SOC15(MMHUB, 0, regMMVM_L2_PROTECTION_FAULT_DEFAULT_ADDR_LO32,
			(uint32_t)(value >> 12));
	WREG32_SOC15(MMHUB, 0, regMMVM_L2_PROTECTION_FAULT_DEFAULT_ADDR_HI32,
			(uint32_t)((uint64_t)value >> 44));

	tmp = RREG32_SOC15(MMHUB, 0, regMMVM_L2_PROTECTION_FAULT_CNTL2);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_PROTECTION_FAULT_CNTL2,
				    ACTIVE_PAGE_MIGRATION_PTE_READ_RETRY, 1);
	WREG32_SOC15(MMHUB, 0, regMMVM_L2_PROTECTION_FAULT_CNTL2, tmp);
}

static void mmhub_v3_0_init_tlb_regs(struct amdgv_adapter *adapt)
{
	uint32_t tmp;

	/* Setup TLB control */
	tmp = RREG32_SOC15(MMHUB, 0, regMMMC_VM_MX_L1_TLB_CNTL);

	tmp = REG_SET_FIELD(tmp, MMMC_VM_MX_L1_TLB_CNTL, ENABLE_L1_TLB, 1);
	tmp = REG_SET_FIELD(tmp, MMMC_VM_MX_L1_TLB_CNTL, SYSTEM_ACCESS_MODE, 3);
	tmp = REG_SET_FIELD(tmp, MMMC_VM_MX_L1_TLB_CNTL, ENABLE_ADVANCED_DRIVER_MODEL, 1);
	tmp = REG_SET_FIELD(tmp, MMMC_VM_MX_L1_TLB_CNTL, SYSTEM_APERTURE_UNMAPPED_ACCESS, 0);
	tmp = REG_SET_FIELD(tmp, MMMC_VM_MX_L1_TLB_CNTL, ECO_BITS, 0);
	tmp = REG_SET_FIELD(tmp, MMMC_VM_MX_L1_TLB_CNTL, MTYPE, MTYPE_UC); /* UC, uncached */

	WREG32_SOC15(MMHUB, 0, regMMMC_VM_MX_L1_TLB_CNTL, tmp);
}

static void mmhub_v3_0_init_cache_regs(struct amdgv_adapter *adapt)
{
	uint32_t tmp;

	tmp = RREG32_SOC15(MMHUB, 0, regMMVM_L2_CNTL);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_CNTL, ENABLE_L2_CACHE, 1);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_CNTL, ENABLE_L2_FRAGMENT_PROCESSING, 0);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_CNTL, ENABLE_DEFAULT_PAGE_OUT_TO_SYSTEM_MEMORY, 1);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_CNTL, L2_PDE0_CACHE_TAG_GENERATION_MODE, 0);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_CNTL, PDE_FAULT_CLASSIFICATION, 0);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_CNTL, CONTEXT1_IDENTITY_ACCESS_MODE, 1);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_CNTL, IDENTITY_MODE_FRAGMENT_SIZE, 0);
	WREG32_SOC15(MMHUB, 0, regMMVM_L2_CNTL, tmp);

	tmp = RREG32_SOC15(MMHUB, 0, regMMVM_L2_CNTL2);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_CNTL2, INVALIDATE_ALL_L1_TLBS, 1);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_CNTL2, INVALIDATE_L2_CACHE, 1);
	WREG32_SOC15(MMHUB, 0, regMMVM_L2_CNTL2, tmp);

	tmp = regMMVM_L2_CNTL3_DEFAULT;
	tmp = REG_SET_FIELD(tmp, MMVM_L2_CNTL3, BANK_SELECT, 12);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_CNTL3, L2_CACHE_BIGK_FRAGMENT_SIZE, 9);
	WREG32_SOC15(MMHUB, 0, regMMVM_L2_CNTL3, tmp);

	tmp = regMMVM_L2_CNTL4_DEFAULT;
	tmp = REG_SET_FIELD(tmp, MMVM_L2_CNTL4, VMC_TAP_PDE_REQUEST_PHYSICAL, 0);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_CNTL4, VMC_TAP_PTE_REQUEST_PHYSICAL, 0);
	WREG32_SOC15(MMHUB, 0, regMMVM_L2_CNTL4, tmp);

	tmp = regMMVM_L2_CNTL5_DEFAULT;
	tmp = REG_SET_FIELD(tmp, MMVM_L2_CNTL5, L2_CACHE_SMALLK_FRAGMENT_SIZE, 0);
	WREG32_SOC15(MMHUB, 0, regMMVM_L2_CNTL5, tmp);
}

static void mmhub_v3_0_enable_system_domain(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	struct amdgv_vmhub *hub;
	int i;

	tmp = RREG32_SOC15(MMHUB, 0, regMMVM_CONTEXT0_CNTL);
	tmp = REG_SET_FIELD(tmp, MMVM_CONTEXT0_CNTL, ENABLE_CONTEXT, 1);
	tmp = REG_SET_FIELD(tmp, MMVM_CONTEXT0_CNTL, PAGE_TABLE_DEPTH, 1);
	tmp = REG_SET_FIELD(tmp, MMVM_CONTEXT0_CNTL, PAGE_TABLE_BLOCK_SIZE, 12);
	tmp = REG_SET_FIELD(tmp, MMVM_CONTEXT0_CNTL,
			    RETRY_PERMISSION_OR_INVALID_PAGE_FAULT, 0);
	WREG32_SOC15(MMHUB, 0, regMMVM_CONTEXT0_CNTL, tmp);

	hub = &adapt->vmhub[AMDGV_MMHUB0(0)];
	if (!hub->eng_addr_distance)
		return;

	for (i = 0; i < 18; ++i) {
		WREG32_SOC15_OFFSET(MMHUB, 0, regMMVM_INVALIDATE_ENG0_ADDR_RANGE_LO32,
				i * hub->eng_addr_distance, 0xffffffff);
		WREG32_SOC15_OFFSET(MMHUB, 0, regMMVM_INVALIDATE_ENG0_ADDR_RANGE_HI32,
				i * hub->eng_addr_distance, 0x1f);
	}
}

static void mmhub_v3_0_disable_identity_aperture(struct amdgv_adapter *adapt)
{
	WREG32_SOC15(MMHUB, 0,
		regMMVM_L2_CONTEXT1_IDENTITY_APERTURE_LOW_ADDR_LO32,
		0xFFFFFFFF);
	WREG32_SOC15(MMHUB, 0,
		regMMVM_L2_CONTEXT1_IDENTITY_APERTURE_LOW_ADDR_HI32,
		0x0000000F);

	WREG32_SOC15(MMHUB, 0, regMMVM_L2_CONTEXT1_IDENTITY_APERTURE_HIGH_ADDR_LO32, 0);
	WREG32_SOC15(MMHUB, 0, regMMVM_L2_CONTEXT1_IDENTITY_APERTURE_HIGH_ADDR_HI32, 0);

	WREG32_SOC15(MMHUB, 0, regMMVM_L2_CONTEXT_IDENTITY_PHYSICAL_OFFSET_LO32, 0);
	WREG32_SOC15(MMHUB, 0, regMMVM_L2_CONTEXT_IDENTITY_PHYSICAL_OFFSET_HI32, 0);
}

static void mmhub_v3_0_set_fault_enable_default(struct amdgv_adapter *adapt, bool value)
{
	uint32_t tmp;

	tmp = RREG32_SOC15(MMHUB, 0, regMMVM_L2_PROTECTION_FAULT_CNTL);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_PROTECTION_FAULT_CNTL,
			    RANGE_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_PROTECTION_FAULT_CNTL,
			    PDE0_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_PROTECTION_FAULT_CNTL,
			    PDE1_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_PROTECTION_FAULT_CNTL,
			    PDE2_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_PROTECTION_FAULT_CNTL,
			    TRANSLATE_FURTHER_PROTECTION_FAULT_ENABLE_DEFAULT,
			    value);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_PROTECTION_FAULT_CNTL,
			    NACK_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_PROTECTION_FAULT_CNTL,
			    DUMMY_PAGE_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_PROTECTION_FAULT_CNTL,
			    VALID_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_PROTECTION_FAULT_CNTL,
			    READ_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_PROTECTION_FAULT_CNTL,
			    WRITE_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	tmp = REG_SET_FIELD(tmp, MMVM_L2_PROTECTION_FAULT_CNTL,
			    EXECUTE_PROTECTION_FAULT_ENABLE_DEFAULT, value);
	if (!value) {
		tmp = REG_SET_FIELD(tmp, MMVM_L2_PROTECTION_FAULT_CNTL,
				CRASH_ON_NO_RETRY_FAULT, 1);
		tmp = REG_SET_FIELD(tmp, MMVM_L2_PROTECTION_FAULT_CNTL,
				CRASH_ON_RETRY_FAULT, 1);
	}
	WREG32_SOC15(MMHUB, 0, regMMVM_L2_PROTECTION_FAULT_CNTL, tmp);
}

void mmhub_v3_0_gart_enable(struct amdgv_adapter *adapt)
{
	mmhub_v3_0_init_gart_aperture_regs(adapt);
	mmhub_v3_0_init_system_aperture_regs(adapt);
	mmhub_v3_0_init_tlb_regs(adapt);
	mmhub_v3_0_init_cache_regs(adapt);

	mmhub_v3_0_enable_system_domain(adapt);
	mmhub_v3_0_disable_identity_aperture(adapt);
	mmhub_v3_0_set_fault_enable_default(adapt, true);
}

void mmhub_v3_0_gart_fini(struct amdgv_adapter *adapt)
{
	uint32_t tmp;

	/* regMMVM_L2_CNTL */
	tmp = RREG32(SOC15_REG_OFFSET(MMHUB, 0, regMMVM_L2_CNTL));
	tmp = REG_SET_FIELD(tmp, MMVM_L2_CNTL, ENABLE_L2_CACHE, 0);
	WREG32(SOC15_REG_OFFSET(MMHUB, 0, regMMVM_L2_CNTL), tmp);

	/* regMMVM_L2_CNTL3 */
	WREG32(SOC15_REG_OFFSET(MMHUB, 0, regMMVM_L2_CNTL3), 0);
}

static uint32_t mmhub_v3_0_get_invalidate_req(unsigned int vmid, uint32_t flush_type)
{
	uint32_t req = 0;

	/* invalidate using legacy mode on vmid*/
	req = REG_SET_FIELD(req, MMVM_INVALIDATE_ENG0_REQ, PER_VMID_INVALIDATE_REQ, 1 << vmid);
	req = REG_SET_FIELD(req, MMVM_INVALIDATE_ENG0_REQ, FLUSH_TYPE, flush_type);
	req = REG_SET_FIELD(req, MMVM_INVALIDATE_ENG0_REQ, INVALIDATE_L2_PTES, 1);
	req = REG_SET_FIELD(req, MMVM_INVALIDATE_ENG0_REQ, INVALIDATE_L2_PDE0, 1);
	req = REG_SET_FIELD(req, MMVM_INVALIDATE_ENG0_REQ, INVALIDATE_L2_PDE1, 1);
	req = REG_SET_FIELD(req, MMVM_INVALIDATE_ENG0_REQ, INVALIDATE_L2_PDE2, 1);
	req = REG_SET_FIELD(req, MMVM_INVALIDATE_ENG0_REQ, INVALIDATE_L1_PTES, 1);
	req = REG_SET_FIELD(req, MMVM_INVALIDATE_ENG0_REQ, CLEAR_PROTECTION_FAULT_STATUS_ADDR, 0);

	return req;
}

static struct amdgv_vmhub_funcs mmhub_v3_0_vmhub_funcs = {
	.get_invalidate_req = mmhub_v3_0_get_invalidate_req,
};

void mmhub_v3_0_init(struct amdgv_adapter *adapt)
{
	struct amdgv_vmhub *hub = &adapt->vmhub[AMDGV_MMHUB0(0)];

	hub->ctx0_ptb_addr_lo32 =
		SOC15_REG_OFFSET(MMHUB, 0,
				 regMMVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32);
	hub->ctx0_ptb_addr_hi32 =
		SOC15_REG_OFFSET(MMHUB, 0,
				 regMMVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_HI32);
	hub->vm_inv_eng0_sem =
		SOC15_REG_OFFSET(MMHUB, 0, regMMVM_INVALIDATE_ENG0_SEM);
	hub->vm_inv_eng0_req =
		SOC15_REG_OFFSET(MMHUB, 0, regMMVM_INVALIDATE_ENG0_REQ);
	hub->vm_inv_eng0_ack =
		SOC15_REG_OFFSET(MMHUB, 0, regMMVM_INVALIDATE_ENG0_ACK);
	hub->vm_context0_cntl =
		SOC15_REG_OFFSET(MMHUB, 0, regMMVM_CONTEXT0_CNTL);
	hub->vm_l2_pro_fault_status =
		SOC15_REG_OFFSET(MMHUB, 0, regMMVM_L2_PROTECTION_FAULT_STATUS);
	hub->vm_l2_pro_fault_cntl =
		SOC15_REG_OFFSET(MMHUB, 0, regMMVM_L2_PROTECTION_FAULT_CNTL);

	hub->ctx_distance = regMMVM_CONTEXT1_CNTL - regMMVM_CONTEXT0_CNTL;
	hub->ctx_addr_distance = regMMVM_CONTEXT1_PAGE_TABLE_BASE_ADDR_LO32 -
		regMMVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32;
	hub->eng_distance = regMMVM_INVALIDATE_ENG1_REQ -
		regMMVM_INVALIDATE_ENG0_REQ;
	hub->eng_addr_distance = regMMVM_INVALIDATE_ENG1_ADDR_RANGE_LO32 -
		regMMVM_INVALIDATE_ENG0_ADDR_RANGE_LO32;

	hub->vm_l2_bank_select_reserved_cid2 = SOC15_REG_OFFSET(MMHUB, 0, regMMVM_L2_BANK_SELECT_RESERVED_CID2);

	hub->vmhub_funcs = &mmhub_v3_0_vmhub_funcs;
}
