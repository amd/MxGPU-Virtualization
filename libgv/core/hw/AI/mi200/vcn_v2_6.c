/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

 /* See MAINTAINERS list for contact information. */
#include "amdgv.h"
#include "mi200/VCN/vcn_2_6_0_offset.h"
#include "mi200/VCN/vcn_2_6_0_sh_mask.h"
#include "amdgv_device.h"
#include "amdgv_ras.h"
#include "amdgv_vcn.h"
#include "vcn_v2_6.h"

static const uint32_t this_block = AMDGV_MULTIMEDIA_BLOCK;

static void vcn_v2_6_query_poison_by_instance(struct amdgv_adapter *adapt,
			uint32_t instance, uint32_t sub_block)
{
	uint32_t poison_stat = 0, reg_value = 0;

	switch (sub_block) {
	case AMDGV_VCN_V2_6_VCPU_VCODEC:
		reg_value = SOC15_REG_OFFSET(VCN, instance,
						regUVD_RAS_VCPU_VCODEC_STATUS);
		break;
	case AMDGV_VCN_V2_6_MMSCH:
		/* MMSCH_FATAL_ERROR is only for VCN0 */
		if (instance)
			return;

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

	return;
}

static void vcn_v2_6_query_poison_status(struct amdgv_adapter *adapt)
{
	uint32_t sub;
	int inst;

	for (inst = 0; inst < adapt->vcn.num_instances; inst++)
		for (sub = 0; sub < AMDGV_VCN_V2_6_MAX_SUB_BLOCK; sub++)
			vcn_v2_6_query_poison_by_instance(adapt, inst, sub);

	return;
}

const struct amdgv_vcn_ras_funcs vcn_v2_6_ras_funcs = {
	.query_poison_status = vcn_v2_6_query_poison_status,
};

void vcn_v2_6_set_ras_funcs(struct amdgv_adapter *adapt)
{
	adapt->vcn.num_instances = 2;
	adapt->vcn.funcs = &vcn_v2_6_ras_funcs;
}
