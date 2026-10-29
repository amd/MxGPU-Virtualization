/*
 * Copyright 2026 Advanced Micro Devices, Inc.
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

#include "amdgv_device.h"
#include "amdgv.h"
#include "amdgv_mmsch.h"
#include "amdgv_vcn.h"
#include "amdgv_nbio.h"

#include "asic_reg/VCN/vcn_5_0_0_sh_mask.h"
#include "asic_reg/VCN/vcn_5_0_0_offset.h"

#include "mmsch_v10_0.h"
#include "psp/psp_v15_0_8.h"
#include "gpuiov/gpuiov_v9_0.h"

static const uint32_t this_block = AMDGV_MULTIMEDIA_BLOCK;

void mmsch_v10_0_get_regid_instid(struct amdgv_adapter *adapt, uint32_t hw_sched_id, uint64_t *reg, bool control)
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

/*
 * MMSCH exposes one copy of each GPUIOV scheduler register per multimedia
 * group: VCN -> *_0, JPEG -> *_1, JPEG1 -> *_2. Pick the offset for the group
 * that owns hw_sched_id (all three copies share the same BASE_IDX).
 */
void mmsch_v10_0_get_gpuiov_reg_by_base(struct amdgv_adapter *adapt,
					uint32_t hw_sched_id, uint64_t *reg,
					uint32_t reg0, uint32_t reg1, uint32_t reg2,
					uint32_t base)
{
	uint32_t inst = adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst;
	uint32_t offset;

	switch (hw_sched_id) {
	case GPUIOV_V9_0_HW_SCHED_BLOCK_VCN_SCH0_MMSCH:
	case GPUIOV_V9_0_HW_SCHED_BLOCK_VCN_SCH1_MMSCH:
	case GPUIOV_V9_0_HW_SCHED_BLOCK_VCN_SCH2_MMSCH:
	case GPUIOV_V9_0_HW_SCHED_BLOCK_VCN_SCH3_MMSCH:
		offset = reg0;
		break;
	case GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH:
	case GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG_SCH1_MMSCH:
	case GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG_SCH2_MMSCH:
	case GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG_SCH3_MMSCH:
		offset = reg1;
		break;
	case GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG1_SCH0_MMSCH:
	case GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG1_SCH1_MMSCH:
	case GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG1_SCH2_MMSCH:
	case GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG1_SCH3_MMSCH:
		offset = reg2;
		break;
	default:
		*reg = 0;
		AMDGV_ERROR("Wrong id -- sched_id=%d (%s) is not a Multimedia id!\n",
			    hw_sched_id, amdgv_hw_sched_id_to_name(adapt, hw_sched_id));
		return;
	}

	*reg = adapt->reg_offset[VCN_HWIP][inst][base] + offset;
}

void mmsch_v10_0_get_time_quanta_index_reg(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
					   uint32_t idx_vf, uint64_t *reg)
{
	if (idx_vf == AMDGV_PF_IDX || idx_vf > 15)
		mmsch_v10_0_get_gpuiov_reg(adapt, hw_sched_id, reg, regMMSCH_GPUIOV_DW8);
	else
		mmsch_v10_0_get_gpuiov_reg(adapt, hw_sched_id, reg, regMMSCH_GPUIOV_DW7);
}

int mmsch_v10_0_set_doorbell_addr_base(struct amdgv_adapter *adapt)
{
	int i, vcn_inst;
	int doorbell_index;
	uint32_t addr;
	uint32_t value;
	uint32_t read_value;

	for (i = 0; i < adapt->config.mm.count[AMDGV_VCN_ENGINE]; i++) {
		vcn_inst = GET_INST(VCN, i);
		doorbell_index = (AMDGV_NBIO_DOORBELL_MMSCH0 << 1) + 32 * vcn_inst;
		addr = SOC15_REG_OFFSET(VCN, vcn_inst, regMMSCH_DB_ADDR_BASE);
		value = doorbell_index << 2;
		WREG32(addr, value);

		read_value = RREG32(addr);
		if (read_value != value) {
			AMDGV_ERROR("Failed to program MMSCH DB_ADDR_BASE for VCN instance %d: expected 0x%x, read 0x%x\n",
				    vcn_inst, value, read_value);
			return AMDGV_FAILURE;
		}
	}

	return 0;
}

static int mmsch_v10_0_check_enabled_features(struct amdgv_adapter *adapt)
{
	// Disable cmd buffer until future usage
	adapt->mmsch.is_feature_enabled.cmd_buffer = false;
	return 0;
}

const struct amdgv_mmsch_funcs mmsch_v10_0_funcs = {
	.check_enabled_features           = mmsch_v10_0_check_enabled_features,
};

static int mmsch_v10_0_sw_init(struct amdgv_adapter *adapt)
{
	adapt->mmsch.cmd_buffer_mem = amdgv_memmgr_alloc_align(
		&adapt->memmgr_pf,
		sizeof(struct amdgv_mmsch_cmd_buffer),
		AMDGV_MMSCH_FB_ALIGNMENT,
		MEM_MMSCH_CMD_BUFFER);

	if (!adapt->mmsch.cmd_buffer_mem) {
		AMDGV_ERROR("Failed to reserve memory for MMSCH command buffer\n");
		goto free;
	}

	adapt->mmsch.sram_dump_mem = amdgv_memmgr_alloc_align(
		&adapt->memmgr_pf,
		AMDGV_MMSCH_SRAM_DUMP_SIZE,
		AMDGV_MMSCH_FB_ALIGNMENT,
		MEM_MMSCH_SRAM_DUMP);

	if (!adapt->mmsch.sram_dump_mem) {
		AMDGV_ERROR("Failed to reserve memory for MMSCH sram dump\n");
		goto free;
	}

	adapt->mmsch.mmsch_funcs = &mmsch_v10_0_funcs;

	return 0;

free:
	if (adapt->mmsch.cmd_buffer_mem)
		amdgv_memmgr_free(adapt->mmsch.cmd_buffer_mem);
	if (adapt->mmsch.sram_dump_mem)
		amdgv_memmgr_free(adapt->mmsch.sram_dump_mem);

	oss_memset(&adapt->mmsch, 0, sizeof(struct amdgv_mmsch));

	return AMDGV_FAILURE;
}

static int mmsch_v10_0_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->mmsch.cmd_buffer_mem)
		amdgv_memmgr_free(adapt->mmsch.cmd_buffer_mem);
	if (adapt->mmsch.sram_dump_mem)
		amdgv_memmgr_free(adapt->mmsch.sram_dump_mem);

	oss_memset(&adapt->mmsch, 0, sizeof(struct amdgv_mmsch));

	return 0;
}

static int mmsch_v10_0_hw_init(struct amdgv_adapter *adapt)
{
	int ret;
	uint64_t mmsch_cmd_buff_mem;
	enum amdgv_firmware_id ucode_id;

	ucode_id = AMDGV_FIRMWARE_ID__MMSCH;
	ret = adapt->ucode.load(adapt, &ucode_id, 1);
	if (ret) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_FW_UCODE_LOAD_FAIL, amdgv_psp_cmd_km_fw_id_map(ucode_id));
		return ret;
	}

	mmsch_v10_0_check_enabled_features(adapt);

	/* set MMSCH command buffer as long as we are before enabling SRIOV */
	mmsch_cmd_buff_mem = amdgv_memmgr_get_gpu_addr(adapt->mmsch.cmd_buffer_mem);
	ret = amdgv_mmsch_set_cmd_buffer_address(adapt, mmsch_cmd_buff_mem);
	if (ret) {
		AMDGV_ERROR("Unable to program MMSCH command buffer. Cannot configure MM engines\n");
		adapt->mmsch.is_feature_enabled.cmd_buffer = false;
		return ret;
	}

	AMDGV_INFO("mmsch buf:0x%llx mmdown:%d size:0x%llx\n",
					mmsch_cmd_buff_mem,
					adapt->mmsch.cmd_buffer_mem->memmgr->down,
					adapt->mmsch.cmd_buffer_mem->memmgr->size);
	AMDGV_INFO("mmsch base:0x%llx offs0:0x%llx offs:0x%llx len:0x%llx\n",
					adapt->mmsch.cmd_buffer_mem->memmgr->mc_base,
					adapt->mmsch.cmd_buffer_mem->memmgr->offset,
					adapt->mmsch.cmd_buffer_mem->alloc_off,
					adapt->mmsch.cmd_buffer_mem->len);

	return 0;
}


static int mmsch_v10_0_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func mmsch_v10_0_func = {
	.name = "mmsch_v10_0_func",
	.sw_init = mmsch_v10_0_sw_init,
	.sw_fini = mmsch_v10_0_sw_fini,
	.hw_init = mmsch_v10_0_hw_init,
	.hw_fini = mmsch_v10_0_hw_fini,
};
