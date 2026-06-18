/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv.h"
#include "navi3/VCN/vcn_4_0_0_offset.h"
#include "navi3/VCN/vcn_4_0_0_sh_mask.h"
#include "amdgv_device.h"
#include "amdgv_ras.h"
#include "amdgv_vcn.h"
#include "navi32_vcn.h"

static const uint32_t this_block = AMDGV_MULTIMEDIA_BLOCK;

static uint32_t navi32_vcn_query_poison_by_instance(struct amdgv_adapter *adapt,
			uint32_t instance, uint32_t sub_block)
{
	uint32_t poison_stat = 0, reg_value = 0;

	switch (sub_block) {
	case NAVI32_VCN_VCPU_VCODEC:
		reg_value = SOC15_REG_OFFSET(VCN, instance,
						regUVD_RAS_VCPU_VCODEC_STATUS);
		break;
	case NAVI32_VCN_MMSCH:
		/* MMSCH_FATAL_ERROR is only for VCN0 */
		if (instance)
			return 0;

		reg_value = SOC15_REG_OFFSET(VCN, instance,
						regUVD_RAS_MMSCH_FATAL_ERROR);
		break;
	default:
		break;
	}

	if (reg_value) {
		poison_stat = RREG32_PCIE(reg_value);
		if (poison_stat)
			AMDGV_INFO(
				"RAS Poison detected in VCN%d, sub_block%d, poison_stat:0x%x\n",
				instance, sub_block, poison_stat);
	}

	return poison_stat;
}

static void navi32_vcn_query_poison_status(struct amdgv_adapter *adapt)
{
	uint32_t sub;
	int inst;
	uint32_t poison_stat = 0;

	for (inst = 0; inst < adapt->vcn.num_instances; inst++)
		for (sub = 0; sub < NAVI32_VCN_MAX_SUB_BLOCK; sub++)
			poison_stat +=
				navi32_vcn_query_poison_by_instance(adapt,
						inst, sub);
}

const struct amdgv_vcn_ras_funcs navi32_vcn_ras_funcs = {
	.query_poison_status = navi32_vcn_query_poison_status,
};

void navi32_vcn_set_ras_funcs(struct amdgv_adapter *adapt)
{
	adapt->vcn.num_instances = 2;
	adapt->vcn.funcs = &navi32_vcn_ras_funcs;
}
