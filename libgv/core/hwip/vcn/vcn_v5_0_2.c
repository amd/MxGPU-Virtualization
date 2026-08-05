/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv_device.h"
#include "amdgv.h"
#include "amdgv_vcn.h"

#include "asic_reg/VCN/vcn_5_0_0_sh_mask.h"
#include "asic_reg/VCN/vcn_5_0_0_offset.h"

#include "vcn_v5_0_2.h"
#include "gpuiov/gpuiov_v9_0.h"

static const uint32_t this_block = AMDGV_MULTIMEDIA_BLOCK;

#define VCN_V5_0_2_DOORBELL_VCN_START 0x1B0

void vcn_v5_0_2_get_mmsch_regid_instid(struct amdgv_adapter *adapt, uint32_t hw_sched_id, uint64_t *reg, bool control)
{
	/* Determine MMSCH register
	 * VCN   -> group 0 register
	 * JPEG  -> group 1 register
	 * JPEG1 -> group 2 register
	 */
	switch (hw_sched_id) {
	case GPUIOV_V9_0_HW_SCHED_BLOCK_VCN_SCH0_MMSCH:
	case GPUIOV_V9_0_HW_SCHED_BLOCK_VCN_SCH1_MMSCH:
	case GPUIOV_V9_0_HW_SCHED_BLOCK_VCN_SCH2_MMSCH:
	case GPUIOV_V9_0_HW_SCHED_BLOCK_VCN_SCH3_MMSCH:
		*reg = control ? SOC15_REG_OFFSET(VCN, adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst, regMMSCH_GPUIOV_CMD_CONTROL_0)
				: SOC15_REG_OFFSET(VCN, adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst, regMMSCH_GPUIOV_CMD_STATUS_0);
		break;
	case GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH:
	case GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG_SCH1_MMSCH:
	case GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG_SCH2_MMSCH:
	case GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG_SCH3_MMSCH:
		*reg = control ? SOC15_REG_OFFSET(VCN, adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst, regMMSCH_GPUIOV_CMD_CONTROL_1)
				: SOC15_REG_OFFSET(VCN, adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst, regMMSCH_GPUIOV_CMD_STATUS_1);
		break;
	case GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG1_SCH0_MMSCH:
	case GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG1_SCH1_MMSCH:
	case GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG1_SCH2_MMSCH:
	case GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG1_SCH3_MMSCH:
		*reg = control ? SOC15_REG_OFFSET(VCN, adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst, regMMSCH_GPUIOV_CMD_CONTROL_2)
				: SOC15_REG_OFFSET(VCN, adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst, regMMSCH_GPUIOV_CMD_STATUS_2);
		break;
	default:
		AMDGV_ERROR("Wrong id -- sched_id=%d (%s) is not a Multimedia id!\n", hw_sched_id, amdgv_hw_sched_id_to_name(adapt, hw_sched_id));
		break;
	}
}

void vcn_v5_0_2_set_mmsch_doorbell_addr_base(struct amdgv_adapter *adapt)
{
	int i, vcn_inst;
	int doorbell_index;

	for (i = 0; i < adapt->config.mm.count[AMDGV_VCN_ENGINE]; i++) {
		vcn_inst = GET_INST(VCN, i);
		doorbell_index = (VCN_V5_0_2_DOORBELL_VCN_START << 1) + 32 * vcn_inst;
		WREG32(SOC15_REG_OFFSET(VCN, vcn_inst, regMMSCH_DB_ADDR_BASE), doorbell_index << 2);
	}
}
