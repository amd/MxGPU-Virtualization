/*
 * Copyright (c) 2024 Advanced Micro Devices, Inc. All rights reserved.
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

#include <amdgv_device.h>
#include <amdgv.h>
#include "mi300.h"
#include "mi350_vcn.h"
#include "mi300_gpuiov.h"
#include <mi350/VCN/vcn_5_0_0_offset.h>
#include <mi350/VCN/vcn_5_0_0_sh_mask.h>

static const uint32_t this_block = AMDGV_MULTIMEDIA_BLOCK;

void mi350_vcn_get_mmsch_regid_instid(struct amdgv_adapter *adapt,
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

void mi350_vcn_set_mmsch_doorbell_addr_base(struct amdgv_adapter *adapt)
{
	int i, vcn_inst;
	int doorbell_index;
	int vcns_per_vf = 4;

	switch (adapt->num_vf) {
	case 1:
		vcns_per_vf = 4;
		break;
	case 2:
		vcns_per_vf = 2;
		break;
	case 4:
	case 8:
		vcns_per_vf = 1;
		break;
	default:
		break;
	}

	for (i = 0; i < adapt->config.mm.count[AMDGV_VCN_ENGINE]; i++) {
		vcn_inst = GET_INST(VCN, i);
		/* Map the db address so each VF's vcn0 is set to AMDGV_MI300_DOORBELL_MMSCH0 << 1 */
		doorbell_index = (AMDGV_MI300_DOORBELL_MMSCH0 << 1) + 32 * (vcn_inst % vcns_per_vf);
		WREG32(SOC15_REG_OFFSET(VCN, vcn_inst, regMMSCH_DB_ADDR_BASE),
				doorbell_index << 2);
	}
}

static int mi350_mmsch_check_enabled_features(struct amdgv_adapter *adapt)
{
	/* SHUTDOWN_GPU command is only supported after MI350 MMSCH 9.0.16 (include) */
	if (adapt->psp.fw_info[AMDGV_FIRMWARE_ID__MMSCH] >= 0x09000010)
		adapt->mmsch.is_feature_enabled.support_shutdown_cmd = true;

	return 0;
}

const struct amdgv_mmsch_funcs mi350_mmsch_funcs = {
	.check_enabled_features = mi350_mmsch_check_enabled_features,
};
