/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv.h>
#include "mi300.h"
#include "mi300_vcn.h"
#include "mi300_gpuiov.h"
#include <mi300/VCN/vcn_4_0_3_offset.h>
#include <mi300/VCN/vcn_4_0_3_sh_mask.h>

static const uint32_t this_block = AMDGV_MULTIMEDIA_BLOCK;

void mi300_vcn_get_mmsch_regid_instid(struct amdgv_adapter *adapt,
							uint32_t hw_sched_id, uint64_t *reg, bool control)
{
	/* Determine MMSCH register
	 * VCN   ->  group 0 register
	 * JPEG  ->  group 1 register
	 * JPEG1 ->  group 2 register
	 */
	switch (hw_sched_id) {
	case MI300_HW_SCHED_BLOCK_VCN_SCH0_MMSCH:
	case MI300_HW_SCHED_BLOCK_VCN_SCH1_MMSCH:
	case MI300_HW_SCHED_BLOCK_VCN_SCH2_MMSCH:
	case MI300_HW_SCHED_BLOCK_VCN_SCH3_MMSCH:
		*reg = control ? SOC15_REG_OFFSET(VCN, adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst, regMMSCH_GPUIOV_CMD_CONTROL_0)
						:  SOC15_REG_OFFSET(VCN, adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst, regMMSCH_GPUIOV_CMD_STATUS_0);
		break;
	case MI300_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH:
	case MI300_HW_SCHED_BLOCK_JPEG_SCH1_MMSCH:
	case MI300_HW_SCHED_BLOCK_JPEG_SCH2_MMSCH:
	case MI300_HW_SCHED_BLOCK_JPEG_SCH3_MMSCH:
		*reg = control ? SOC15_REG_OFFSET(VCN, adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst, regMMSCH_GPUIOV_CMD_CONTROL_1)
						:  SOC15_REG_OFFSET(VCN, adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst, regMMSCH_GPUIOV_CMD_STATUS_1);
		break;
	case MI300_HW_SCHED_BLOCK_JPEG1_SCH0_MMSCH:
	case MI300_HW_SCHED_BLOCK_JPEG1_SCH1_MMSCH:
	case MI300_HW_SCHED_BLOCK_JPEG1_SCH2_MMSCH:
	case MI300_HW_SCHED_BLOCK_JPEG1_SCH3_MMSCH:
		*reg = control ? SOC15_REG_OFFSET(VCN, adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst, regMMSCH_GPUIOV_CMD_CONTROL_2)
						:  SOC15_REG_OFFSET(VCN, adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst, regMMSCH_GPUIOV_CMD_STATUS_2);
		break;
	default:
		AMDGV_ERROR("Wrong id -- sched_id=%d (%s) is not a Multimedia id!\n", hw_sched_id, amdgv_hw_sched_id_to_name(adapt, hw_sched_id));
		break;
	}
}
