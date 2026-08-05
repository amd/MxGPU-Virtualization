/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv.h>
#include <amdgv_device.h>
#include <amdgv_gart.h>

#include "mmhub_v4_2_0.h"
#include "asic_reg/MMHUB/mmhub_4_2_0_offset.h"
#include "asic_reg/MMHUB/mmhub_4_2_0_sh_mask.h"

#define MMHUB_V4_2_0_MMVM_L2_CNTL3_DEFAULT 0x80100007
#define MMHUB_V4_2_0_MMVM_L2_CNTL4_DEFAULT 0x000000c1
#define MMHUB_V4_2_0_MMVM_L2_CNTL5_DEFAULT 0x00003fe0

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static void mmhub_v4_2_0_init_system_aperture_regs(struct amdgv_adapter *adapt)
{
	uint32_t i, l2_prot;

	for (i = 0; i < adapt->mmhub.num_instances; i++) {
		if (adapt->xgmi.connected_to_cpu) {
			WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
					regMMMC_VM_SYSTEM_APERTURE_LOW_ADDR_LO32,
					0xFFFFFFFF);
			WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
					regMMMC_VM_SYSTEM_APERTURE_LOW_ADDR_HI32,
					0x7F);
			WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
					regMMMC_VM_SYSTEM_APERTURE_HIGH_ADDR_LO32, 0);
			WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
					regMMMC_VM_SYSTEM_APERTURE_HIGH_ADDR_HI32, 0);
		} else {
			WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMMC_VM_SYSTEM_APERTURE_LOW_ADDR_LO32,
					lower_32_bits(adapt->mc_sys_loc_addr >> 18));
			WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMMC_VM_SYSTEM_APERTURE_LOW_ADDR_HI32,
					upper_32_bits(adapt->mc_sys_loc_addr >> 18));
			WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMMC_VM_SYSTEM_APERTURE_HIGH_ADDR_LO32,
					lower_32_bits(adapt->mc_sys_top_addr >> 18));
			WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMMC_VM_SYSTEM_APERTURE_HIGH_ADDR_HI32,
					upper_32_bits(adapt->mc_sys_top_addr >> 18));
		}
		/* @VICTOR: Is this required? */
		//	WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR_LSB, hdp_nonsurface_base_lo);
		//	WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMC_VM_SYSTEM_APERTURE_DEFAULT_ADDR_MSB, hdp_nonsurface_base_hi);

		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMVM_L2_PROTECTION_FAULT_DEFAULT_ADDR_LO32, 0);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMVM_L2_PROTECTION_FAULT_DEFAULT_ADDR_HI32, 0);

		l2_prot = RREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMVM_L2_PROTECTION_FAULT_CNTL2);
		l2_prot = REG_SET_FIELD(l2_prot, MMVM_L2_PROTECTION_FAULT_CNTL2,
					ACTIVE_PAGE_MIGRATION_PTE_READ_RETRY, 1);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMVM_L2_PROTECTION_FAULT_CNTL2, l2_prot);
	}
}

static void mmhub_v4_2_0_init_tlb_regs(struct amdgv_adapter *adapt)
{
	uint32_t i, l1;

	for (i = 0; i < adapt->mmhub.num_instances; i++) {
		l1 = RREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMMC_VM_MX_L1_TLB_CNTL);
		l1 = REG_SET_FIELD(l1, MMMC_VM_MX_L1_TLB_CNTL, ENABLE_L1_TLB, 1);
		l1 = REG_SET_FIELD(l1, MMMC_VM_MX_L1_TLB_CNTL, SYSTEM_ACCESS_MODE, 3);
		l1 = REG_SET_FIELD(l1, MMMC_VM_MX_L1_TLB_CNTL, ENABLE_ADVANCED_DRIVER_MODEL, 1);
		l1 = REG_SET_FIELD(l1, MMMC_VM_MX_L1_TLB_CNTL, SYSTEM_APERTURE_UNMAPPED_ACCESS, 0);
		l1 = REG_SET_FIELD(l1, MMMC_VM_MX_L1_TLB_CNTL, ECO_BITS, 0);
		l1 = REG_SET_FIELD(l1, MMMC_VM_MX_L1_TLB_CNTL, MTYPE, MTYPE_UC);

		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMMC_VM_MX_L1_TLB_CNTL, l1);
	}
}

static void mmhub_v4_2_0_init_cache_regs(struct amdgv_adapter *adapt)
{
	uint32_t i, l2;

	for (i = 0; i < adapt->mmhub.num_instances; i++) {
		l2 = RREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMVM_L2_CNTL);
		l2 = REG_SET_FIELD(l2, MMVM_L2_CNTL, ENABLE_L2_CACHE, 1);
		l2 = REG_SET_FIELD(l2, MMVM_L2_CNTL, ENABLE_L2_FRAGMENT_PROCESSING, 0);
		l2 = REG_SET_FIELD(l2, MMVM_L2_CNTL, ENABLE_DEFAULT_PAGE_OUT_TO_SYSTEM_MEMORY, 1);
		l2 = REG_SET_FIELD(l2, MMVM_L2_CNTL, L2_PDE0_CACHE_TAG_GENERATION_MODE, 0);
		l2 = REG_SET_FIELD(l2, MMVM_L2_CNTL, PDE_FAULT_CLASSIFICATION, 0);
		l2 = REG_SET_FIELD(l2, MMVM_L2_CNTL, CONTEXT1_IDENTITY_ACCESS_MODE, 1);
		l2 = REG_SET_FIELD(l2, MMVM_L2_CNTL, IDENTITY_MODE_FRAGMENT_SIZE, 0);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMVM_L2_CNTL, l2);

		l2 = RREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMVM_L2_CNTL2);
		l2 = REG_SET_FIELD(l2, MMVM_L2_CNTL2, INVALIDATE_ALL_L1_TLBS, 1);
		l2 = REG_SET_FIELD(l2, MMVM_L2_CNTL2, INVALIDATE_L2_CACHE, 1);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMVM_L2_CNTL2, l2);

		l2 = MMHUB_V4_2_0_MMVM_L2_CNTL3_DEFAULT;
		l2 = REG_SET_FIELD(l2, MMVM_L2_CNTL3, BANK_SELECT, 12);
		l2 = REG_SET_FIELD(l2, MMVM_L2_CNTL3, L2_CACHE_BIGK_FRAGMENT_SIZE, 9);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMVM_L2_CNTL3, l2);

		l2 = MMHUB_V4_2_0_MMVM_L2_CNTL4_DEFAULT;
		if (adapt->xgmi.connected_to_cpu) {
			l2 = REG_SET_FIELD(l2, MMVM_L2_CNTL4, VMC_TAP_PDE_REQUEST_PHYSICAL, 1);
			l2 = REG_SET_FIELD(l2, MMVM_L2_CNTL4, VMC_TAP_PTE_REQUEST_PHYSICAL, 1);
		} else {
			l2 = REG_SET_FIELD(l2, MMVM_L2_CNTL4, VMC_TAP_PDE_REQUEST_PHYSICAL, 0);
			l2 = REG_SET_FIELD(l2, MMVM_L2_CNTL4, VMC_TAP_PTE_REQUEST_PHYSICAL, 0);
		}
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMVM_L2_CNTL4, l2);

		l2 = MMHUB_V4_2_0_MMVM_L2_CNTL5_DEFAULT;
		l2 = REG_SET_FIELD(l2, MMVM_L2_CNTL5, L2_CACHE_SMALLK_FRAGMENT_SIZE, 0);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMVM_L2_CNTL5, l2);
	}
}

static void mmhub_v4_2_0_disable_identity_aperture(struct amdgv_adapter *adapt)
{
	uint32_t i;

	for (i = 0; i < adapt->mmhub.num_instances; i++) {
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
			     regMMVM_L2_CONTEXT1_IDENTITY_APERTURE_LOW_ADDR_LO32,0XFFFFFFFF);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
			     regMMVM_L2_CONTEXT1_IDENTITY_APERTURE_LOW_ADDR_HI32, 0x00001FFF);

		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
			     regMMVM_L2_CONTEXT1_IDENTITY_APERTURE_HIGH_ADDR_LO32, 0);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
			     regMMVM_L2_CONTEXT1_IDENTITY_APERTURE_HIGH_ADDR_HI32, 0);

		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
			     regMMVM_L2_CONTEXT_IDENTITY_PHYSICAL_OFFSET_LO32, 0);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
			     regMMVM_L2_CONTEXT_IDENTITY_PHYSICAL_OFFSET_HI32, 0);
	}
}

static void mmhub_v4_2_0_set_fault_enable_default(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t i, l2_prot;

	for (i = 0; i < adapt->mmhub.num_instances; i++) {
		l2_prot = RREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
				       regMMVM_L2_PROTECTION_FAULT_CNTL_LO32);
		l2_prot = REG_SET_FIELD(l2_prot, MMVM_L2_PROTECTION_FAULT_CNTL_LO32,
					RANGE_PROTECTION_FAULT_ENABLE_DEFAULT, enable);
		l2_prot = REG_SET_FIELD(l2_prot, MMVM_L2_PROTECTION_FAULT_CNTL_LO32,
					PDE0_PROTECTION_FAULT_ENABLE_DEFAULT, enable);
		l2_prot = REG_SET_FIELD(l2_prot, MMVM_L2_PROTECTION_FAULT_CNTL_LO32,
					PDE1_PROTECTION_FAULT_ENABLE_DEFAULT, enable);
		l2_prot = REG_SET_FIELD(l2_prot, MMVM_L2_PROTECTION_FAULT_CNTL_LO32,
					PDE2_PROTECTION_FAULT_ENABLE_DEFAULT, enable);
		l2_prot = REG_SET_FIELD(l2_prot, MMVM_L2_PROTECTION_FAULT_CNTL_LO32,
					TRANSLATE_FURTHER_PROTECTION_FAULT_ENABLE_DEFAULT, enable);
		l2_prot = REG_SET_FIELD(l2_prot, MMVM_L2_PROTECTION_FAULT_CNTL_LO32,
					NACK_PROTECTION_FAULT_ENABLE_DEFAULT, enable);
		l2_prot = REG_SET_FIELD(l2_prot, MMVM_L2_PROTECTION_FAULT_CNTL_LO32,
					DUMMY_PAGE_PROTECTION_FAULT_ENABLE_DEFAULT, enable);
		l2_prot = REG_SET_FIELD(l2_prot, MMVM_L2_PROTECTION_FAULT_CNTL_LO32,
					VALID_PROTECTION_FAULT_ENABLE_DEFAULT, enable);
		l2_prot = REG_SET_FIELD(l2_prot, MMVM_L2_PROTECTION_FAULT_CNTL_LO32,
					READ_PROTECTION_FAULT_ENABLE_DEFAULT, enable);
		l2_prot = REG_SET_FIELD(l2_prot, MMVM_L2_PROTECTION_FAULT_CNTL_LO32,
					WRITE_PROTECTION_FAULT_ENABLE_DEFAULT, enable);
		l2_prot = REG_SET_FIELD(l2_prot, MMVM_L2_PROTECTION_FAULT_CNTL_LO32,
					EXECUTE_PROTECTION_FAULT_ENABLE_DEFAULT, enable);
		if (!enable) {
			l2_prot = REG_SET_FIELD(l2_prot, MMVM_L2_PROTECTION_FAULT_CNTL_LO32,
						CRASH_ON_NO_RETRY_FAULT, 1);
		}

		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMVM_L2_PROTECTION_FAULT_CNTL_LO32,
			     l2_prot);

		if (!enable) {
			l2_prot = RREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
				       regMMVM_L2_PROTECTION_FAULT_CNTL_HI32);
			l2_prot = REG_SET_FIELD(l2_prot, MMVM_L2_PROTECTION_FAULT_CNTL_HI32,
						CRASH_ON_RETRY_FAULT, 1);
			WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMVM_L2_PROTECTION_FAULT_CNTL_HI32,
			     l2_prot);
		}
	}
}

static int mmhub_v4_2_0_get_xgmi_info(struct amdgv_adapter *adapt)
{
	uint32_t max_num_physical_nodes;
	uint32_t max_physical_node_id;
	uint32_t xgmi_lfb_cntl;
	uint32_t max_region;
	uint64_t seg_size;

	xgmi_lfb_cntl = RREG32_SOC15(MMHUB, GET_INST(MMHUB, 0),
				     regMMMC_VM_XGMI_LFB_CNTL);
	seg_size = REG_GET_FIELD(
		RREG32_SOC15(MMHUB, GET_INST(MMHUB, 0), regMMMC_VM_XGMI_LFB_SIZE),
		MMMC_VM_XGMI_LFB_SIZE, PF_LFB_SIZE);
	seg_size <<= 24;
	max_region =
		REG_GET_FIELD(xgmi_lfb_cntl, MMMC_VM_XGMI_LFB_CNTL, PF_MAX_REGION);

	max_num_physical_nodes   = 4;
	max_physical_node_id     = 3;

	adapt->xgmi.phy_nodes_num = max_region + 1;

	if (adapt->xgmi.phy_nodes_num > max_num_physical_nodes)
		return -1;

	adapt->xgmi.phy_node_id =
		REG_GET_FIELD(xgmi_lfb_cntl, MMMC_VM_XGMI_LFB_CNTL, PF_LFB_REGION);

	if (adapt->xgmi.phy_node_id > max_physical_node_id)
		return -1;

	adapt->xgmi.node_segment_size = seg_size;

	AMDGV_DEBUG("XGMI info - phy_nodes_num: %d, phy_node_id: %d, node_segment_size: 0x%llx\n",
		    adapt->xgmi.phy_nodes_num, adapt->xgmi.phy_node_id, adapt->xgmi.node_segment_size);

	return 0;
}

static uint64_t mmhub_v4_2_0_get_mc_fb_offset(struct amdgv_adapter *adapt)
{
	uint64_t offset;

	offset = RREG32(SOC15_REG_OFFSET(MMHUB, GET_INST(MMHUB, 0), regMMMC_VM_FB_OFFSET));
	offset <<= 24;

	return offset;
}

static int mmhub_v4_2_0_get_fb_location(struct amdgv_adapter *adapt, uint64_t *base, uint64_t *top)
{
	uint64_t tmp, tmp_hi;

	tmp = RREG32(SOC15_REG_OFFSET(MMHUB, GET_INST(MMHUB, 0), regMMMC_VM_FB_LOCATION_BASE_LO32));
	tmp = REG_GET_FIELD(tmp, MMMC_VM_FB_LOCATION_BASE_LO32, FB_BASE_LO32);
	tmp_hi = RREG32(SOC15_REG_OFFSET(MMHUB, GET_INST(MMHUB, 0), regMMMC_VM_FB_LOCATION_BASE_HI32));
	tmp_hi = REG_GET_FIELD(tmp_hi, MMMC_VM_FB_LOCATION_BASE_HI32, FB_BASE_HI1);

	tmp = tmp + (tmp_hi << 32);
	*base = tmp << MC_VM_FB_LOCATION__FB_ADDRESS__SHIFT;

	tmp = RREG32(SOC15_REG_OFFSET(MMHUB, GET_INST(MMHUB, 0), regMMMC_VM_FB_LOCATION_TOP_LO32));
	tmp = REG_GET_FIELD(tmp, MMMC_VM_FB_LOCATION_TOP_LO32, FB_TOP_LO32);
	tmp_hi = RREG32(SOC15_REG_OFFSET(MMHUB, GET_INST(MMHUB, 0), regMMMC_VM_FB_LOCATION_TOP_HI32));
	tmp_hi = REG_GET_FIELD(tmp_hi, MMMC_VM_FB_LOCATION_TOP_HI32, FB_TOP_HI1);

	tmp = tmp + (tmp_hi << 32);
	*top = tmp << MC_VM_FB_LOCATION__FB_ADDRESS__SHIFT;

	return 0;
}

static void mmhub_v4_2_0_setup_pt_regs(struct amdgv_adapter *adapt)
{
	uint64_t page_table_base = amdgv_memmgr_get_gpu_pa(adapt->pdb0_mem);
	uint32_t i;

	page_table_base |= AMDGV_PTE_VALID;
	page_table_base |= AMDGV_PTE_SNOOPED;

	for (i = 0; i < adapt->mmhub.num_instances; i++) {
		WREG32_SOC15_OFFSET(MMHUB, GET_INST(MMHUB, i),
				    regMMVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32,
				    0,
				    lower_32_bits(page_table_base));

		WREG32_SOC15_OFFSET(MMHUB, GET_INST(MMHUB, i),
				    regMMVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_HI32,
				    0,
				    upper_32_bits(page_table_base));
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
					regMMVM_CONTEXT0_PAGE_TABLE_START_ADDR_LO32,
					(uint32_t)(GART_START >> 12));
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
					regMMVM_CONTEXT0_PAGE_TABLE_START_ADDR_HI32,
					(uint32_t)(GART_START >> 44));
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
					regMMVM_CONTEXT0_PAGE_TABLE_END_ADDR_LO32,
					(uint32_t)((adapt->gart_size + GART_START + adapt->fb_size) >> 12));
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
					regMMVM_CONTEXT0_PAGE_TABLE_END_ADDR_HI32,
					(uint32_t)((adapt->gart_size + GART_START + adapt->fb_size) >> 44));
	}
}

static void mmhub_v4_2_0_init_gart_aperture_regs(struct amdgv_adapter *adapt)
{
	uint32_t i;

	mmhub_v4_2_0_setup_pt_regs(adapt);

	/* disable AGP, FB aperture */
	for (i = 0; i < adapt->mmhub.num_instances; i++) {
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
					regMMMC_VM_FB_LOCATION_TOP_LO32, 0);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
					regMMMC_VM_FB_LOCATION_TOP_HI32, 0);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
					regMMMC_VM_FB_LOCATION_BASE_LO32, 0xFFFFFFFF);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
					regMMMC_VM_FB_LOCATION_BASE_HI32, 1);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
					regMMMC_VM_AGP_TOP_LO32, 0);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
					regMMMC_VM_AGP_TOP_HI32, 0);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
					regMMMC_VM_AGP_BOT_LO32, 0xFFFFFFFF);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
					regMMMC_VM_AGP_BOT_HI32, 1);
	}
}

static void mmhub_v4_2_0_enable_system_domain(struct amdgv_adapter *adapt)
{
	uint32_t tmp;
	int i;

	for (i = 0; i < adapt->mmhub.num_instances; i++) {
		tmp = RREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
				   regMMVM_CONTEXT0_CNTL);
		tmp = REG_SET_FIELD(tmp, MMVM_CONTEXT0_CNTL,
				    ENABLE_CONTEXT, 1);
		tmp = REG_SET_FIELD(tmp, MMVM_CONTEXT0_CNTL,
				    PAGE_TABLE_DEPTH, 1);
		tmp = REG_SET_FIELD(tmp, MMVM_CONTEXT0_CNTL,
				    PAGE_TABLE_BLOCK_SIZE, 12);
		tmp = REG_SET_FIELD(tmp, MMVM_CONTEXT0_CNTL,
				    RETRY_PERMISSION_OR_INVALID_PAGE_FAULT, 0);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i),
			     regMMVM_CONTEXT0_CNTL, tmp);
	}
}

static int mmhub_v4_2_0_gart_enable(struct amdgv_adapter *adapt)
{
	if (adapt->xgmi.connected_to_cpu) {
		mmhub_v4_2_0_init_gart_aperture_regs(adapt);
	}
	mmhub_v4_2_0_init_system_aperture_regs(adapt);
	mmhub_v4_2_0_init_tlb_regs(adapt);
	mmhub_v4_2_0_init_cache_regs(adapt);

	if (adapt->xgmi.connected_to_cpu) {
		mmhub_v4_2_0_enable_system_domain(adapt);
	}

	mmhub_v4_2_0_disable_identity_aperture(adapt);
	mmhub_v4_2_0_set_fault_enable_default(adapt, !is_debug_mode_hang());

	return 0;
}

static int mmhub_v4_2_0_gart_disable(struct amdgv_adapter *adapt)
{
	uint32_t l1, l2, i;

	for (i = 0; i < adapt->mmhub.num_instances; i++) {
		l1 = RREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMMC_VM_MX_L1_TLB_CNTL);
		l1 = REG_SET_FIELD(l1, MMMC_VM_MX_L1_TLB_CNTL, ENABLE_L1_TLB, 0);
		l1 = REG_SET_FIELD(l1, MMMC_VM_MX_L1_TLB_CNTL, ENABLE_ADVANCED_DRIVER_MODEL, 0);
		l1 = REG_SET_FIELD(l1, MMMC_VM_MX_L1_TLB_CNTL, ENABLE_ADVANCED_DRIVER_MODEL, 0);
		l1 = REG_SET_FIELD(l1, MMMC_VM_MX_L1_TLB_CNTL, SYSTEM_APERTURE_UNMAPPED_ACCESS, 0);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMMC_VM_MX_L1_TLB_CNTL, l1);

		l2 = RREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMVM_L2_CNTL);
		l2 = REG_SET_FIELD(l2, MMVM_L2_CNTL, ENABLE_L2_CACHE, 0);
		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMVM_L2_CNTL, l2);

		WREG32_SOC15(MMHUB, GET_INST(MMHUB, i), regMMVM_L2_CNTL3, 0);
	}

	return 0;
}

static void mmhub_v4_2_0_clear_protection_fault(struct amdgv_adapter *adapt, uint32_t inst)
{
	WREG32_SOC15(MMHUB, GET_INST(MMHUB, inst),
			 regMMVM_L2_PROTECTION_FAULT_ADDR_HI32, 0);
	WREG32_SOC15(MMHUB, GET_INST(MMHUB, inst),
			 regMMVM_L2_PROTECTION_FAULT_ADDR_LO32, 0);
	WREG32_SOC15(MMHUB, GET_INST(MMHUB, inst),
			 regMMVM_L2_PROTECTION_FAULT_STATUS_HI32, 0);
	WREG32_SOC15(MMHUB, GET_INST(MMHUB, inst),
			 regMMVM_L2_PROTECTION_FAULT_STATUS_LO32, 0);
}

static const struct amdgv_mmhub_funcs mmhub_v4_2_0_funcs = {
	.query_ras_error_count = NULL, /* Do not support legacy query. Go through mca_count_cache */
	.get_mc_fb_offset = mmhub_v4_2_0_get_mc_fb_offset,
	.get_fb_location = mmhub_v4_2_0_get_fb_location,
	.gart_enable = mmhub_v4_2_0_gart_enable,
	.gart_disable = mmhub_v4_2_0_gart_disable,
	.get_xgmi_info = mmhub_v4_2_0_get_xgmi_info,
};

static uint32_t mmhub_v4_2_get_invalidate_req(unsigned int vmid, uint32_t flush_type)
{
	uint32_t req = 0;

	req = REG_SET_FIELD(req, MMVM_INVALIDATE_ENG0_REQ, PER_VMID_INVALIDATE_REQ, 1 << vmid);
	req = REG_SET_FIELD(req, MMVM_INVALIDATE_ENG0_REQ, FLUSH_TYPE, flush_type);
	req = REG_SET_FIELD(req, MMVM_INVALIDATE_ENG0_REQ, INVALIDATE_L2_PTES, 1);
	req = REG_SET_FIELD(req, MMVM_INVALIDATE_ENG0_REQ, INVALIDATE_L2_PDE0, 1);
	req = REG_SET_FIELD(req, MMVM_INVALIDATE_ENG0_REQ, INVALIDATE_L2_PDE1, 1);
	req = REG_SET_FIELD(req, MMVM_INVALIDATE_ENG0_REQ, INVALIDATE_L2_PDE2, 1);
	req = REG_SET_FIELD(req, MMVM_INVALIDATE_ENG0_REQ, INVALIDATE_L2_PDE3, 1);
	req = REG_SET_FIELD(req, MMVM_INVALIDATE_ENG0_REQ, INVALIDATE_L1_PTES, 1);
	req = REG_SET_FIELD(req, MMVM_INVALIDATE_ENG0_REQ, CLEAR_PROTECTION_FAULT_STATUS_ADDR, 0);

	return req;
}

static struct amdgv_vmhub_funcs mmhub_v4_2_vmhub_funcs = {
	.get_invalidate_req = mmhub_v4_2_get_invalidate_req,
	.clear_protection_fault = mmhub_v4_2_0_clear_protection_fault,
};

static int mmhub_v4_2_0_set_funcs(struct amdgv_adapter *adapt)
{
	adapt->mmhub.funcs = &mmhub_v4_2_0_funcs;

	return 0;
}

int mmhub_v4_2_0_sw_init(struct amdgv_adapter *adapt)
{
	struct amdgv_vmhub *hub;
	int i;

	mmhub_v4_2_0_set_funcs(adapt);

	for (i = 0; i < adapt->mcp.num_aid; i++) {
		hub = &adapt->vmhub[AMDGV_MMHUB0(i)];

		hub->ctx0_ptb_addr_lo32 = SOC15_REG_OFFSET(MMHUB, i,
			regMMVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32);
		hub->ctx0_ptb_addr_hi32 = SOC15_REG_OFFSET(MMHUB, i,
			regMMVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_HI32);
		hub->vm_inv_eng0_req =
			SOC15_REG_OFFSET(MMHUB, i, regMMVM_INVALIDATE_ENG0_REQ);
		hub->vm_inv_eng0_ack =
			SOC15_REG_OFFSET(MMHUB, i, regMMVM_INVALIDATE_ENG0_ACK);
		hub->vm_context0_cntl =
			SOC15_REG_OFFSET(MMHUB, i, regMMVM_CONTEXT0_CNTL);

		hub->vm_l2_pro_fault_status =
			SOC15_REG_OFFSET(MMHUB, i, regMMVM_L2_PROTECTION_FAULT_STATUS_LO32);
		hub->vm_l2_pro_fault_status_hi =
			SOC15_REG_OFFSET(MMHUB, i, regMMVM_L2_PROTECTION_FAULT_STATUS_HI32);

		hub->ctx_distance = regMMVM_CONTEXT1_CNTL - regMMVM_CONTEXT0_CNTL;
		hub->ctx_addr_distance =
			regMMVM_CONTEXT1_PAGE_TABLE_BASE_ADDR_LO32 -
			regMMVM_CONTEXT0_PAGE_TABLE_BASE_ADDR_LO32;
		hub->eng_distance = regMMVM_INVALIDATE_ENG1_REQ -
			regMMVM_INVALIDATE_ENG0_REQ;
		hub->eng_addr_distance = regMMVM_INVALIDATE_ENG1_ADDR_RANGE_LO32 -
			regMMVM_INVALIDATE_ENG0_ADDR_RANGE_LO32;

		hub->vm_l2_bank_select_reserved_cid2 =
			SOC15_REG_OFFSET(MMHUB, GET_INST(MMHUB, i),
			regMMVM_L2_BANK_SELECT_RESERVED_CID2);

		hub->vmhub_funcs = &mmhub_v4_2_vmhub_funcs;
	}

	return 0;
}

static int mmhub_v4_2_0_sw_fini(struct amdgv_adapter *adapt) {
	return 0;
}

static int mmhub_v4_2_0_hw_init(struct amdgv_adapter *adapt) {

	return amdgv_mmhub_gart_enable(adapt);
}

static int mmhub_v4_2_0_hw_fini(struct amdgv_adapter *adapt) {

	return amdgv_mmhub_gart_disable(adapt);
}

const struct amdgv_init_func mmhub_v4_2_0_func = {
	.name = "mmhub_v4_2_0_func",
	.sw_init = mmhub_v4_2_0_sw_init,
	.sw_fini = mmhub_v4_2_0_sw_fini,
	.hw_init = mmhub_v4_2_0_hw_init,
	.hw_fini = mmhub_v4_2_0_hw_fini,
};