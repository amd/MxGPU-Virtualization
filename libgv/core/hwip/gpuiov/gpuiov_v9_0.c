/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv_gpuiov.h>
#include <amdgv_psp.h>
#include <amdgv.h>
#include <amdgv_iovm_drv.h>
#include "gpuiov_v9_0.h"
#include "vcn/vcn_v5_0_2.h"
#include "asic_reg/GC/gc_12_1_0_offset.h"
#include "asic_reg/GC/gc_12_1_0_sh_mask.h"
#include "asic_reg/NBIO/nbio_6_3_2_offset.h"
#include "asic_reg/NBIO/nbio_6_3_2_sh_mask.h"

static const int this_block = AMDGV_COMMUNICATION_BLOCK;


static const struct gpuiov_v9_0_cmd_id gpuiov_v9_0_cmd_array[] = {
	{"IDLE",			AMDGV_IDLE_GPU,				GPUIOV_V9_0_IDLE_GPU},
	{"SAVE",			AMDGV_SAVE_GPU_STATE,			GPUIOV_V9_0_SAVE_GPU_STATE},
	{"LOAD",			AMDGV_LOAD_GPU_STATE,			GPUIOV_V9_0_LOAD_GPU_STATE},
	{"RUN",				AMDGV_RUN_GPU,				GPUIOV_V9_0_RUN_GPU},
	{"CONTEXT SWITCH",		AMDGV_CONTEXT_SWITCH,			GPUIOV_V9_0_CONTEXT_SWITCH},
	{"ENABLE HW_AUTO_SCHED",	AMDGV_ENABLE_AUTO_HW_SWITCH,		GPUIOV_V9_0_ENABLE_AUTO_SCHEDULING},
	{"INIT",			AMDGV_INIT_GPU,				GPUIOV_V9_0_INIT_GPU},
	{"DISABLE HW_AUTO_SCHED",	AMDGV_DISABLE_AUTO_HW_SCHED,		GPUIOV_V9_0_DISABLE_AUTO_SCHEDULING},
	{"SHUTDOWN VF",			AMDGV_SHUTDOWN_GPU,			GPUIOV_V9_0_SHUTDOWN_GPU},
	{"CONFIG HW_AUTO_SCHED_MODE",	AMDGV_CONFIG_AUTO_HW_SCHED_MODE,	GPUIOV_V9_0_CONFIG_AUTO_SCHEDULING},
	{"EVENT NOTIFICATION",		AMDGV_EVENT_NOTIFICATION,		GPUIOV_V9_0_EVENT_NOTIFICATION},
	{"TRANSFER VF DATA",		AMDGV_TRANSFER_VF_DATA,			GPUIOV_V9_0_TRANSFER_VF_DATA},
};

static struct amdgv_gpuiov_hw_sched_static_config gpuiov_v9_0_static_config[] = {
	{"VCN_SCH0_MMSCH",	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_VCN,	AMDGV_SCHED_FRAME_LOOP_MODE,	PCI_GPUIOV_VCN_SCH0_OFFSET,	0, BIT(AMDGV_SCHED_FRAME_LOOP_MODE)},
	{"JPEG_SCH0_MMSCH",	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_JPEG,	AMDGV_SCHED_FRAME_LOOP_MODE,	PCI_GPUIOV_JPEG_SCH0_OFFSET,	0, BIT(AMDGV_SCHED_FRAME_LOOP_MODE)},
	{"JPEG1_SCH0_MMSCH",	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_JPEG,	AMDGV_SCHED_FRAME_LOOP_MODE,	PCI_GPUIOV_JPEG1_SCH0_OFFSET,	0, BIT(AMDGV_SCHED_FRAME_LOOP_MODE)},
	{"VCN_SCH1_MMSCH",	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_VCN,	AMDGV_SCHED_FRAME_LOOP_MODE,	PCI_GPUIOV_VCN_SCH1_OFFSET,	1, BIT(AMDGV_SCHED_FRAME_LOOP_MODE)},
	{"JPEG_SCH1_MMSCH",	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_JPEG,	AMDGV_SCHED_FRAME_LOOP_MODE,	PCI_GPUIOV_JPEG_SCH1_OFFSET,	1, BIT(AMDGV_SCHED_FRAME_LOOP_MODE)},
	{"JPEG1_SCH1_MMSCH",	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_JPEG,	AMDGV_SCHED_FRAME_LOOP_MODE,	PCI_GPUIOV_JPEG1_SCH1_OFFSET,	1, BIT(AMDGV_SCHED_FRAME_LOOP_MODE)},
	{"VCN_SCH2_MMSCH",	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_VCN,	AMDGV_SCHED_FRAME_LOOP_MODE,	PCI_GPUIOV_VCN_SCH2_OFFSET,	2, BIT(AMDGV_SCHED_FRAME_LOOP_MODE)},
	{"JPEG_SCH2_MMSCH",	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_JPEG,	AMDGV_SCHED_FRAME_LOOP_MODE,	PCI_GPUIOV_JPEG_SCH2_OFFSET,	2, BIT(AMDGV_SCHED_FRAME_LOOP_MODE)},
	{"JPEG1_SCH2_MMSCH",	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_JPEG,	AMDGV_SCHED_FRAME_LOOP_MODE,	PCI_GPUIOV_JPEG1_SCH2_OFFSET,	2, BIT(AMDGV_SCHED_FRAME_LOOP_MODE)},
	{"VCN_SCH3_MMSCH",	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_VCN,	AMDGV_SCHED_FRAME_LOOP_MODE,	PCI_GPUIOV_VCN_SCH3_OFFSET,	3, BIT(AMDGV_SCHED_FRAME_LOOP_MODE)},
	{"JPEG_SCH3_MMSCH",	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_JPEG,	AMDGV_SCHED_FRAME_LOOP_MODE,	PCI_GPUIOV_JPEG_SCH3_OFFSET,	3, BIT(AMDGV_SCHED_FRAME_LOOP_MODE)},
	{"JPEG1_SCH3_MMSCH",	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_JPEG,	AMDGV_SCHED_FRAME_LOOP_MODE,	PCI_GPUIOV_JPEG1_SCH3_OFFSET,	3, BIT(AMDGV_SCHED_FRAME_LOOP_MODE)},
	{"GFX_SCH0_RLCV",	AMDGV_HW_SCHED_TYPE_GFX,	AMDGV_SCHED_BLOCK_GFX,	AMDGV_SCHED_FAIRNESS,		PCI_GPUIOV_GFX_SCH0_OFFSET,	0, GPUIOV_V9_0_SUPPORTED_GFX_SCHED_MODE},
	{"GFX_SCH1_RLCV",	AMDGV_HW_SCHED_TYPE_GFX,	AMDGV_SCHED_BLOCK_GFX,	AMDGV_SCHED_FAIRNESS,		PCI_GPUIOV_GFX_SCH1_OFFSET,	1, GPUIOV_V9_0_SUPPORTED_GFX_SCHED_MODE},
	{"GFX_SCH2_RLCV",	AMDGV_HW_SCHED_TYPE_GFX,	AMDGV_SCHED_BLOCK_GFX,	AMDGV_SCHED_FAIRNESS,		PCI_GPUIOV_GFX_SCH2_OFFSET,	2, GPUIOV_V9_0_SUPPORTED_GFX_SCHED_MODE},
	{"GFX_SCH3_RLCV",	AMDGV_HW_SCHED_TYPE_GFX,	AMDGV_SCHED_BLOCK_GFX,	AMDGV_SCHED_FAIRNESS,		PCI_GPUIOV_GFX_SCH3_OFFSET,	3, GPUIOV_V9_0_SUPPORTED_GFX_SCHED_MODE},
	{"GFX_SCH4_RLCV",	AMDGV_HW_SCHED_TYPE_GFX,	AMDGV_SCHED_BLOCK_GFX,	AMDGV_SCHED_FAIRNESS,		PCI_GPUIOV_GFX_SCH4_OFFSET,	4, GPUIOV_V9_0_SUPPORTED_GFX_SCHED_MODE},
	{"GFX_SCH5_RLCV",	AMDGV_HW_SCHED_TYPE_GFX,	AMDGV_SCHED_BLOCK_GFX,	AMDGV_SCHED_FAIRNESS,		PCI_GPUIOV_GFX_SCH5_OFFSET,	5, GPUIOV_V9_0_SUPPORTED_GFX_SCHED_MODE},
	{"GFX_SCH6_RLCV",	AMDGV_HW_SCHED_TYPE_GFX,	AMDGV_SCHED_BLOCK_GFX,	AMDGV_SCHED_FAIRNESS,		PCI_GPUIOV_GFX_SCH6_OFFSET,	6, GPUIOV_V9_0_SUPPORTED_GFX_SCHED_MODE},
	{"GFX_SCH7_RLCV",	AMDGV_HW_SCHED_TYPE_GFX,	AMDGV_SCHED_BLOCK_GFX,	AMDGV_SCHED_FAIRNESS,		PCI_GPUIOV_GFX_SCH7_OFFSET,	7, GPUIOV_V9_0_SUPPORTED_GFX_SCHED_MODE},
};

static int gpuiov_v9_0_find_cap(struct amdgv_adapter *adapt)
{
	int pos = 0, found = 0;
	uint32_t vsec_id;

	/* search GPUIOV capability */
	while ((pos = oss_pci_find_next_ext_cap(adapt->dev, pos, PCI_EXT_CAP_ID_VNDR)) != 0) {
		uint32_t vsec = 0;
		oss_pci_read_config_dword(adapt->dev, pos + PCI_GPUIOV_VSEC, &vsec);
		vsec_id = PCI_GPUIOV_VSEC__ID(vsec);

		if (vsec_id == PCI_GPUIOV_VSEC__ID__GPU_IOV) {
			found = 1;
			break;
		}
	}

	if (!found) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_IOV_ASIC_NO_SRIOV_SUPPORT, 0);
		return 0;
	}

	return pos;
}

static int gpuiov_v9_0_get_sched_block_offset(struct amdgv_adapter *adapt,
					       uint32_t hw_sched_id)
{

	if (hw_sched_id >= adapt->gpuiov.num_ctrl_blocks) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_SCHED_INVALID_HW_SCHED_ID, hw_sched_id);
		return AMDGV_FAILURE;
	}

	return (adapt->gpuiov.pos + (adapt->gpuiov.ctrl_blocks[hw_sched_id].offset << 4));
}


static enum gpuiov_v9_0_cmd gpuiov_v9_0_decode_cmd(enum amdgv_gpuiov_cmd cmd)
{
	uint32_t i;
	uint32_t size = ARRAY_SIZE(gpuiov_v9_0_cmd_array);

	for (i = 0; i < size; ++i) {
		if (gpuiov_v9_0_cmd_array[i].cmd == cmd)
			return gpuiov_v9_0_cmd_array[i].gpuiov_v9_0_cmd;
	}

	return GPUIOV_V9_0_INVALID_COMMAND;
}

static const char *gpuiov_v9_0_cmd_to_name(struct amdgv_adapter *adapt, uint32_t cmd, uint32_t hw_sched_id)
{
	if (IS_HW_SCHED_TYPE_GFX(hw_sched_id)) {
		uint32_t i;
		uint32_t size = ARRAY_SIZE(gpuiov_v9_0_cmd_array);

		for (i = 0; i < size; ++i) {
			if (gpuiov_v9_0_cmd_array[i].cmd == cmd)
				return gpuiov_v9_0_cmd_array[i].name;
		}
		return "INVALID CMD";
	}

	return amdgv_gpuiov_cmd_to_name_default(adapt, cmd, hw_sched_id);
}

static int gpuiov_v9_0_set_cmd(struct amdgv_adapter *adapt,
				 enum amdgv_gpuiov_cmd cmd,
				 uint32_t hw_sched_id,
				 uint32_t idx_vf, uint32_t next_idx_vf)
{
	uint32_t data, func_id, next_func_id;
	uint64_t reg_control = 0;
	enum gpuiov_v9_0_cmd cmd_v9_0;
	uint32_t xcd_bitmask = 0;

	func_id = PCI_GPUIOV_FUNC_ID(idx_vf);
	next_func_id = PCI_GPUIOV_FUNC_ID(next_idx_vf);

	if (IS_HW_SCHED_TYPE_GFX(hw_sched_id)) {
		cmd_v9_0 = gpuiov_v9_0_decode_cmd(cmd);
		if (cmd_v9_0 == GPUIOV_V9_0_INVALID_COMMAND)
			goto out;

		/* Use idx of vf directly, otherwise decoding needed in RLCV */
		if (cmd_v9_0 == GPUIOV_V9_0_CONFIG_AUTO_SCHEDULING) {
			func_id = idx_vf;
			next_func_id = next_idx_vf;
		}
		cmd = cmd_v9_0;
	}

	data = (cmd & 0x0F) | CMD_EXECUTE | (func_id << 8) | (next_func_id << 16);

	if (!oss_atomic_read(adapt->in_sync_flood)) {
		if (IS_HW_SCHED_TYPE_MM(hw_sched_id)) {
			/* TODO: Move to SR-IOV driver*/
			vcn_v5_0_2_get_mmsch_regid_instid(adapt, hw_sched_id, &reg_control, true);
			WREG32(reg_control, data);
		} else {
			if (adapt->iovm_drv.enabled)
			{
				/* TODO: Revisit for multi XCD optimization */
				oss_mutex_lock(adapt->gpuiov.lock);
				xcd_bitmask |= BIT(adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst);
				amdgv_iovm_drv_gpuiov_set_command(adapt, xcd_bitmask, 0, idx_vf, next_idx_vf, cmd);
				oss_mutex_unlock(adapt->gpuiov.lock);
				goto out;
			}
		}
		AMDGV_INFO("GPUIOV command: idx_vf=%d sched_id=%d (%s) cmd=0x%x (%s) func_id=0x%x "
				"next_func_id=0x%x\n",
				idx_vf, hw_sched_id, amdgv_hw_sched_id_to_name(adapt, hw_sched_id), cmd,
				gpuiov_v9_0_cmd_to_name(adapt, cmd, hw_sched_id), func_id, next_func_id);
	} else {
		AMDGV_DEBUG("In in_sync_flood. Skip idx_vf=%d sched_id=%d (%s) cmd=0x%x (%s) func_id=0x%x "
				"next_func_id=0x%x\n",
				idx_vf, hw_sched_id, amdgv_hw_sched_id_to_name(adapt, hw_sched_id), cmd,
				gpuiov_v9_0_cmd_to_name(adapt, cmd, hw_sched_id), func_id, next_func_id);
	}

out:
	adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd = cmd;
	adapt->gpuiov.ctrl_blocks[hw_sched_id].last_status = AMDGV_CMD_STATUS_PENDING_EXECUTE;
	return 0;
}

static bool gpuiov_v9_0_is_cmd_complete(struct amdgv_adapter *adapt,
					uint32_t hw_sched_id)
{
	uint8_t command, status;
	uint64_t reg_control, reg_status;
	//uint32_t xcd_bitmask = 0;

	reg_control = 0;
	reg_status = 0;

	if (IS_HW_SCHED_TYPE_MM(hw_sched_id)) {
		vcn_v5_0_2_get_mmsch_regid_instid(adapt, hw_sched_id, &reg_control, true);
		vcn_v5_0_2_get_mmsch_regid_instid(adapt, hw_sched_id, &reg_status, false);
		command = RREG32(reg_control);
		status = RREG32(reg_status) & 0xFF;
	} else {
		/*
		if (adapt->iovm_drv.enabled)
		{
			// TODO: Revisit for multi XCD optimization
			// TODO: Add MMSCH Support
			return (amdgv_iovm_drv_gpuiov_query_status(adapt, xcd_bitmask, 0) == 0);
		}
		*/
		command = RREG32(SOC15_REG_OFFSET(GC, adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst,
						  regRLC_GPU_IOV_CFG_REG1));
		status = RREG32(SOC15_REG_OFFSET(GC, adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst,
						 regRLC_RLCS_IOV_CMD_STATUS)) & 0xFF;
	}
	adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd = command;
	adapt->gpuiov.ctrl_blocks[hw_sched_id].last_status = status;

	return ((!(command & CMD_EXECUTE)) && status == 0);
}

static void gpuiov_v9_0_dump_cmd_status(struct amdgv_adapter *adapt, uint32_t hw_sched_id)
{
	uint8_t command, status;

	command = adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd;
	status = adapt->gpuiov.ctrl_blocks[hw_sched_id].last_status;

	AMDGV_INFO("hw_sched_id=%d (%s) GPUIOV_CMD=0x%x (%s) CMD_STATUS=0x%x (%s)\n", hw_sched_id,
		    amdgv_hw_sched_id_to_name(adapt, hw_sched_id), command,
		    gpuiov_v9_0_cmd_to_name(adapt, command, hw_sched_id), status,
		    amdgv_gpuiov_status_to_name(status));
}

static int gpuiov_v9_0_set_total_fb_consumed(struct amdgv_adapter *adapt,
					      uint16_t total_fb_consumed)
{
	uint32_t offset = adapt->gpuiov.pos + PCI_GPUIOV_TOTAL_FB_CONSUMED;

	return oss_pci_write_config_word(adapt->dev, offset, total_fb_consumed);
}

static int gpuiov_v9_0_set_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf,
				  uint32_t fb_offset, uint32_t fb_size)
{
	uint32_t i;
	uint32_t reg;

	/*256 MB offset granularity, 16 MB size granularity */
	uint32_t data = ((fb_size >> 4)) | ((fb_offset >> 8) << 16);
	amdgv_put_log(idx_vf, AMDGV_LOG_SCHED_SET_VF_FB,
		      AMDGV_LOG_DATA_32_32(fb_offset, fb_size));

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
		reg = SOC15_REG_OFFSET(GC, GET_INST(GC, i), regGCMC_VM_FB_SIZE_OFFSET_VF0) + idx_vf;
		WREG32(reg, data);
	}

	adapt->array_vf[idx_vf].real_fb_size = fb_size;

	return 0;
}

static int gpuiov_v9_0_get_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf,
				  uint32_t *fb_offset, uint32_t *fb_size, uint32_t *real_fb_size)
{
	uint32_t data;
	uint32_t inst, offset;

	/* Pick the first XCC in mask to read the register from */
	inst = amdgv_ffs(amdgv_sched_get_xcc_mask_by_vf(adapt, idx_vf)) - 1;

	/* VF registers are defined in a continuous range */
	offset = SOC15_REG_OFFSET(GC, GET_INST(GC, inst), regGCMC_VM_FB_SIZE_OFFSET_VF0);
	offset += idx_vf;
	data = RREG32(offset);

	*fb_offset = ((data & 0xFFFF0000) >> 16) << 4;
	*real_fb_size = (data & 0x0000FFFF) << 8;
	*fb_size = *real_fb_size;

	return 0;
}

static int gpuiov_v9_0_get_vm_busy_status(struct amdgv_adapter *adapt,
					   uint32_t hw_sched_id,
					   uint32_t *vm_busy_status)
{
	int offset = gpuiov_v9_0_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE)
		return AMDGV_FAILURE;

	offset += PCI_SCH_VM_BUSY_STATUS;

	return oss_pci_read_config_dword(adapt->dev, offset, vm_busy_status);
}

static int gpuiov_v9_0_clear_intr_status(struct amdgv_adapter *adapt, uint32_t bits)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_INTR_STATUS;

	return oss_pci_write_config_dword(adapt->dev, offset, bits);
}

static int gpuiov_v9_0_get_intr_bits(struct amdgv_adapter *adapt, uint32_t *bits)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_INTR_ENABLE;
	return oss_pci_read_config_dword(adapt->dev, offset, bits);
}

static int gpuiov_v9_0_set_intr_bits(struct amdgv_adapter *adapt, uint32_t bits)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_INTR_ENABLE;

	return oss_pci_write_config_dword(adapt->dev, offset, bits);
}

static int gpuiov_v9_0_get_intr_status(struct amdgv_adapter *adapt, uint32_t *bits)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_INTR_STATUS;

	return oss_pci_read_config_dword(adapt->dev, offset, bits);
}

static int gpuiov_v9_0_get_hvvm_mbox_index(struct amdgv_adapter *adapt, uint8_t *index)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_HVVM_MBOX0;

	return oss_pci_read_config_byte(adapt->dev, offset, index);
}

static int gpuiov_v9_0_update_hvvm_mbox_index(struct amdgv_adapter *adapt, uint8_t index)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_HVVM_MBOX0;

	return oss_pci_write_config_byte(adapt->dev, offset, index);
}

static int gpuiov_v9_0_rcv_hvvm_mbox_msg(struct amdgv_adapter *adapt, uint8_t *msg_data)
{
	int ret;
	uint32_t reg;
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_HVVM_MBOX0;

	ret = oss_pci_read_config_dword(adapt->dev, offset, &reg);
	if (ret != 0)
		return ret;

	*msg_data = (reg & 0xf0000) >> 16;

	return 0;
}

static int gpuiov_v9_0_trn_hvvm_mbox_data(struct amdgv_adapter *adapt, uint8_t msg_data)
{
	int ret;
	uint32_t reg;
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_HVVM_MBOX0;

	ret = oss_pci_read_config_dword(adapt->dev, offset, &reg);
	if (ret != 0)
		return ret;

	reg &= ~0xf0000;
	reg = reg | ((msg_data << 16) & 0xf0000);
	ret = oss_pci_write_config_dword(adapt->dev, offset, reg);

	return ret;
}

static int gpuiov_v9_0_set_hvvm_mbox_valid(struct amdgv_adapter *adapt, uint8_t bits)
{
	int ret;
	uint32_t reg;
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_HVVM_MBOX0;

	ret = oss_pci_read_config_dword(adapt->dev, offset, &reg);
	if (ret != 0)
		return ret;

	/* bits maybe 0 */
	reg &= ~(0x1 << 15);
	reg |= (bits & 0x1) << 15;
	ret = oss_pci_write_config_dword(adapt->dev, offset, reg);

	return ret;
}

static int gpuiov_v9_0_set_hvvm_mbox_ack(struct amdgv_adapter *adapt)
{
	int ret;
	uint32_t reg;
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_HVVM_MBOX0;

	ret = oss_pci_read_config_dword(adapt->dev, offset, &reg);
	if (ret != 0)
		return ret;

	reg |= 0x1 << 24;
	ret = oss_pci_write_config_dword(adapt->dev, offset, reg);

	return ret;
}

static int gpuiov_v9_0_get_hvvm_mbox_msg_valid(struct amdgv_adapter *adapt, uint32_t *bit_map)
{
	int ret;
	uint32_t idx;
	uint32_t valid_bits, offset;

	*bit_map = 0;
	offset = adapt->gpuiov.pos + PCI_GPUIOV_HVVM_MBOX1;
	ret = oss_pci_read_config_dword(adapt->dev, offset, &valid_bits);
	if (ret != 0)
		return ret;

	for (idx = 0; idx < 16; idx++) {
		if (valid_bits & 2)
			*bit_map |= 1 << idx;

		valid_bits >>= 2;
	}

	offset = adapt->gpuiov.pos + PCI_GPUIOV_HVVM_MBOX2;
	ret = oss_pci_read_config_dword(adapt->dev, offset, &valid_bits);
	if (ret != 0)
		return ret;

	if (valid_bits & 2)
		*bit_map |= 1 << 17;

	return 0;
}

static int gpuiov_v9_0_get_active_vfs(struct amdgv_adapter *adapt,
				       uint32_t hw_sched_id,
				       uint32_t *active_vfs)
{
	int offset;

	offset = gpuiov_v9_0_get_sched_block_offset(adapt, hw_sched_id) +
		 PCI_SCH_ACTIVE_FUNCTIONS;
	oss_pci_read_config_dword(adapt->dev, offset, active_vfs);

	return 0;
}

static int gpuiov_v9_0_set_active_vfs(struct amdgv_adapter *adapt,
				       uint32_t hw_sched_id,
				       uint32_t active_vfs)
{
	int offset;

	offset = gpuiov_v9_0_get_sched_block_offset(adapt, hw_sched_id) +
		 PCI_SCH_ACTIVE_FUNCTIONS;
	oss_pci_write_config_dword(adapt->dev, offset, active_vfs);

	return 0;
}

static int gpuiov_v9_0_remove_active_vf(struct amdgv_adapter *adapt,
					 uint32_t hw_sched_id,
					 uint32_t idx_vf)
{
	uint32_t active_vfs = 0;

	gpuiov_v9_0_get_active_vfs(adapt, hw_sched_id, &active_vfs);

	if (idx_vf == AMDGV_PF_IDX)
		active_vfs &= ~(1U << 31);
	else
		active_vfs &= ~(1U << idx_vf);

	gpuiov_v9_0_set_active_vfs(adapt, hw_sched_id, active_vfs);
	return 0;
}

static int gpuiov_v9_0_get_active_vf_idx(struct amdgv_adapter *adapt,
					  uint32_t hw_sched_id,
					  uint32_t *idx_vf)
{
	int offset;
	uint32_t data;

	offset = gpuiov_v9_0_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE)
		return AMDGV_FAILURE;

	offset += PCI_SCH_ACTIVE_FUNCTION_ID;
	oss_pci_read_config_dword(adapt->dev, offset, &data);

	switch (adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_sched_type) {
	case AMDGV_HW_SCHED_TYPE_GFX:
		if (data & 0x80000000)
			*idx_vf = data & 0x1F;
		else
			*idx_vf = AMDGV_PF_IDX;
		break;
	case AMDGV_HW_SCHED_TYPE_MM:
		if (data & 0x80)
			*idx_vf = data & 0x1F;
		else
			*idx_vf = AMDGV_PF_IDX;
		break;

	default:
		return AMDGV_FAILURE;
	}

	AMDGV_DEBUG("PCI_SCH_ACTIVE_FUNCTION_ID: readback=0x%08x, idx_vf=%d\n", data, *idx_vf);

	return 0;
}

static int gpuiov_v9_0_get_active_vf_status(struct amdgv_adapter *adapt,
					     uint32_t hw_sched_id,
					     uint8_t *status)
{
	int offset;

	offset = gpuiov_v9_0_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE)
		return AMDGV_FAILURE;

	offset += PCI_SCH_ACTIVE_FUNCTION_ID_STATUS;
	oss_pci_read_config_byte(adapt->dev, offset, status);

	return 0;
}

static int gpuiov_v9_0_get_time_quanta_index(struct amdgv_adapter *adapt,
					      uint32_t idx_vf,
					      uint32_t hw_sched_id,
					      uint32_t *time_quanta_index)
{
	uint32_t data;
	int offset;

	offset = gpuiov_v9_0_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE)
		return AMDGV_FAILURE;

	if (idx_vf == AMDGV_PF_IDX) {
		offset += PCI_SCH_TIME_QUANTA_PF;
		oss_pci_read_config_dword(adapt->dev, offset, &data);
		*time_quanta_index = PCI_SCH_TIME_QUANTA_PF__GET(data);
	} else {
		offset += PCI_SCH_TIME_QUANTA_PER_VF;
		oss_pci_read_config_dword(adapt->dev, offset, &data);
		*time_quanta_index = (data >> (idx_vf * 2)) & 0x3;
	}

	return 0;
}

static int gpuiov_v9_0_set_time_quanta_index(struct amdgv_adapter *adapt,
					      uint32_t idx_vf,
					      uint32_t hw_sched_id,
					      uint32_t time_quanta_index)
{
	uint32_t data;
	int offset;

	offset = gpuiov_v9_0_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE)
		return AMDGV_FAILURE;

	if (idx_vf == AMDGV_PF_IDX) {
		offset += PCI_SCH_TIME_QUANTA_PF;
		data = PCI_SCH_TIME_QUANTA_PF__SET(time_quanta_index);
		oss_pci_write_config_dword(adapt->dev, offset, data);
	} else {
		offset += PCI_SCH_TIME_QUANTA_PER_VF;
		oss_pci_read_config_dword(adapt->dev, offset, &data);

		data &= ~((0x3) << (idx_vf * 2));
		data |= ((time_quanta_index & 0x3) << (idx_vf * 2));

		oss_pci_write_config_dword(adapt->dev, offset, data);
	}

	return 0;
}

static int gpuiov_v9_0_get_time_quanta_definition(struct amdgv_adapter *adapt,
					uint32_t hw_sched_id,
					int index, uint8_t *quanta_option)
{
	uint32_t data;
	int offset;

	offset = gpuiov_v9_0_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE)
		return AMDGV_FAILURE;

	offset += PCI_SCH_TIME_QUANTA_OPTION;
	oss_pci_read_config_dword(adapt->dev, offset, &data);
	*quanta_option = (data >> (index * 8)) & 0xff;

	return 0;
}

static int gpuiov_v9_0_set_time_quanta_definition(struct amdgv_adapter *adapt,
					uint32_t hw_sched_id,
					int index, uint8_t quanta_option)
{
	uint32_t data;
	int offset;

	offset = gpuiov_v9_0_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE)
		return AMDGV_FAILURE;

	offset += PCI_SCH_TIME_QUANTA_OPTION;
	oss_pci_read_config_dword(adapt->dev, offset, &data);

	data &= ~(0xff << (index * 8));
	data |= quanta_option << (index * 8);

	oss_pci_write_config_dword(adapt->dev, offset, data);
	return 0;
}

static int gpuiov_v9_0_add_active_vf(struct amdgv_adapter *adapt,
				      uint32_t hw_sched_id,
				      uint32_t idx_vf)
{
	uint32_t active_vfs = 0;

	gpuiov_v9_0_get_active_vfs(adapt, hw_sched_id, &active_vfs);

	if (idx_vf == AMDGV_PF_IDX)
		active_vfs |= (1U << 31);
	else
		active_vfs |= (1U << idx_vf);

	gpuiov_v9_0_set_active_vfs(adapt, hw_sched_id, active_vfs);
	return 0;
}

static int gpuiov_v9_0_get_time_quanta_option(struct amdgv_adapter *adapt,
					    uint32_t hw_sched_id,
					    uint32_t *time_quanta_option)
{
	int offset;

	offset = gpuiov_v9_0_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE)
		return AMDGV_FAILURE;

	offset += PCI_SCH_TIME_QUANTA_OPTION;
	oss_pci_read_config_dword(adapt->dev, offset, time_quanta_option);
	return 0;
}

static int gpuiov_v9_0_set_time_quanta_option(struct amdgv_adapter *adapt,
					    uint32_t hw_sched_id,
					    uint32_t time_quanta_option)
{
	int offset;

	offset = gpuiov_v9_0_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE)
		return AMDGV_FAILURE;

	offset += PCI_SCH_TIME_QUANTA_OPTION;
	oss_pci_write_config_dword(adapt->dev, offset, time_quanta_option);

	return 0;
}

static int gpuiov_v9_0_set_vf_access(struct amdgv_adapter *adapt,
				      uint32_t idx_vf,
				      uint32_t vf_access_select, bool is_true)
{
	uint32_t ret = 0;
	uint32_t access_mode = 0;
	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;
	if (idx_vf == AMDGV_PF_IDX)
		return 0;
	if (adapt->flags & AMDGV_FLAG_DISABLE_MMIO_PROTECTION) {
		AMDGV_DEBUG("force disable vf mmio protection\n");
		vf_access_select = AMDGV_VF_ACCESS_ALL;
		is_true = true;
	}

	if (vf_access_select & AMDGV_VF_ACCESS_FB) {
		if (is_true)
			access_mode |= IOVM_DRV_VF_FB_ENABLE_MASK;
		else
			access_mode |= IOVM_DRV_VF_FB_DISABLE_MASK;
	}

	if (vf_access_select & AMDGV_VF_ACCESS_DOORBELL) {
		if (is_true)
			access_mode |= IOVM_DRV_VF_DOORBELL_ENABLE_MASK;
		else
			access_mode |= IOVM_DRV_VF_DOORBELL_DISABLE_MASK;
	}

	if (vf_access_select & AMDGV_VF_ACCESS_MMIO_REG_WRITE) {
		if (is_true)
			access_mode |= IOVM_DRV_VF_MMR_FULL_ACCESS_MASK;
		else
			access_mode |= IOVM_DRV_VF_MMR_RANGE_ACCESS_MASK;
		access_mode |= IOVM_DRV_VF_MMR_WRITE_ACCESS_MASK;
		access_mode |= IOVM_DRV_VF_MMR_READ_ACCESS_MASK;
	}
	ret = amdgv_iovm_drv_vf_access(adapt, idx_vf, access_mode);

	return ret;
}

static bool gpuiov_v9_0_get_vf_access(struct amdgv_adapter *adapt, uint32_t idx_vf,
				      uint32_t vf_access_select)
{
	uint32_t tmp, value;

	if (idx_vf == AMDGV_PF_IDX)
		return false;
	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return false;

	value = (uint32_t)0x1 << idx_vf;

	if (vf_access_select & AMDGV_VF_ACCESS_FB)
		tmp = RREG32_SOC15(NBIO, 0, regBIF_BX0_VF_FB_EN);
	else if (vf_access_select & AMDGV_VF_ACCESS_DOORBELL)
		tmp = RREG32_SOC15(NBIO, 0, regBIF_BX0_VF_DOORBELL_EN);
	else if (vf_access_select & AMDGV_VF_ACCESS_MMIO_REG_WRITE)
		tmp = RREG32_SOC15(NBIO, 0, regBIF_BX0_VF_REGWR_EN);
	else
		return false;

	return tmp & value;
}

static int gpuiov_v9_0_get_fb_info(struct amdgv_adapter *adapt)
{
	uint64_t memmgr_size = 0;

	if (adapt->memmgr_gpu.is_init) {
		amdgv_memmgr_get_limit(&adapt->memmgr_gpu, &memmgr_size);
	}

	/* read total available framebuffer */
	adapt->gpuiov.total_fb_avail = amdgv_nbio_get_memsize(adapt);

	/* Before enable ip discovery, reserve memmgr size */
	adapt->gpuiov.total_fb_usable = adapt->gpuiov.total_fb_avail - (memmgr_size >> 20);

	amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_GPUMON_FB_INFO,
		      AMDGV_LOG_DATA_32_32(adapt->gpuiov.total_fb_avail,
					   adapt->gpuiov.total_fb_usable));

	return 0;
}

static bool gpuiov_v9_0_skip_reading_sch_offset(struct amdgv_adapter *adapt, uint32_t hw_sched_id)
{
	if (IS_HW_SCHED_TYPE_GFX(hw_sched_id))
		return !(adapt->mcp.gfx.xcc_mask & (1 << adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst));

	return false;
}

static void gpuiov_v9_0_get_sch_offset(struct amdgv_adapter *adapt)
{
	uint32_t offset, i;

	for (i = 0; i < adapt->gpuiov.num_ctrl_blocks; i++) {
		if (gpuiov_v9_0_skip_reading_sch_offset(adapt, i))
			continue;

		offset = adapt->gpuiov.pos + adapt->gpuiov.ctrl_blocks[i].pci_gpuiov_offset;
		oss_pci_read_config_byte(adapt->dev, offset, &adapt->gpuiov.ctrl_blocks[i].offset);

		AMDGV_DEBUG("pci offset = 0x%x %s block offset = 0x%x\n", offset,\
			    adapt->gpuiov.ctrl_blocks[i].name, 		\
			    adapt->gpuiov.ctrl_blocks[i].offset << 4);
	}
}

static int gpuiov_v9_0_get_config_info(struct amdgv_adapter *adapt)
{
	adapt->gpuiov.pos = gpuiov_v9_0_find_cap(adapt);
	AMDGV_DEBUG("pci cap offset = 0x%x\n", adapt->gpuiov.pos);

	if (adapt->gpuiov.pos == 0)
		return AMDGV_FAILURE;

	if (gpuiov_v9_0_get_fb_info(adapt))
		return AMDGV_FAILURE;

	gpuiov_v9_0_get_sch_offset(adapt);

	if (oss_pci_find_ext_cap(adapt->dev, PCIE_EXT_CAP_ID__REBAR) != 0)
		adapt->vf_rebar_en = true;

	return 0;
}

static int gpuiov_v9_0_wait_auto_sched_stop(struct amdgv_adapter *adapt,
						uint32_t hw_sched_id)
{
	struct amdgv_sched_world_switch *world_switch;
	int offset;
	int wait_ret;

	if (amdgv_sched_get_world_switch_by_hw_sched_id(adapt, hw_sched_id, &world_switch))
		return AMDGV_FAILURE;

	if (!world_switch->enabled)
		return AMDGV_FAILURE;

	offset = gpuiov_v9_0_get_sched_block_offset(adapt, hw_sched_id);

	if (offset == AMDGV_FAILURE)
		return AMDGV_FAILURE;

	offset += PCI_SCH_ACTIVE_FUNCTION_ID_STATUS;
	wait_ret = amdgv_wait_for_pci_cfg(adapt, adapt->dev, offset,
			0xf, 0, 1, AMDGV_TIMEOUT(TIMEOUT_AUTO_SWITCH_MM), AMDGV_WAIT_CHECK_EQ, 0);

	if (wait_ret)
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_SCHED_AUTO_SCHED_STOP_TIMEOUT,
			      hw_sched_id);

	return wait_ret;
}

static bool gpuiov_v9_0_skip_ctrl_block(struct amdgv_adapter *adapt, int idx)
{
	return (gpuiov_v9_0_static_config[idx].hw_sched_type == AMDGV_HW_SCHED_TYPE_GFX &&
			!(adapt->mcp.gfx.xcc_mask & (1 << gpuiov_v9_0_static_config[idx].hw_inst)));
}

static void gpuiov_v9_0_toggle_vf_mse(struct amdgv_adapter *adapt, bool enable)
{
	uint32_t sriov_ctrl = 0;

	oss_pci_read_config_dword(adapt->dev,
			adapt->sriov_cap_pos + PCIE_EXT_SRIOV_CTRL, &sriov_ctrl);
	if (enable)
		sriov_ctrl |= PCIE_EXT_SRIOV_CTRL_MSE;
	else
		sriov_ctrl &= ~PCIE_EXT_SRIOV_CTRL_MSE;

	oss_pci_write_config_dword(adapt->dev,
			adapt->sriov_cap_pos + PCIE_EXT_SRIOV_CTRL, sriov_ctrl);
}

static const struct amdgv_gpuiov_funcs gpuiov_v9_0_funcs = {
	.set_cmd = gpuiov_v9_0_set_cmd,
	.cmd_to_name = gpuiov_v9_0_cmd_to_name,
	.is_cmd_complete = gpuiov_v9_0_is_cmd_complete,
	.dump_gpuiov_cmd_status = gpuiov_v9_0_dump_cmd_status,
	.set_total_fb_consumed = gpuiov_v9_0_set_total_fb_consumed,
	.set_vf_fb = gpuiov_v9_0_set_vf_fb,
	.get_vf_fb = gpuiov_v9_0_get_vf_fb,
	.get_vm_busy_status = gpuiov_v9_0_get_vm_busy_status,
	.get_intr_bits = gpuiov_v9_0_get_intr_bits,
	.set_intr_bits = gpuiov_v9_0_set_intr_bits,
	.get_intr_status = gpuiov_v9_0_get_intr_status,
	.get_hvvm_mbox_index = gpuiov_v9_0_get_hvvm_mbox_index,
	.update_hvvm_mbox_index = gpuiov_v9_0_update_hvvm_mbox_index,
	.rcv_hvvm_mbox_msg = gpuiov_v9_0_rcv_hvvm_mbox_msg,
	.trn_hvvm_mbox_data = gpuiov_v9_0_trn_hvvm_mbox_data,
	.set_hvvm_mbox_valid = gpuiov_v9_0_set_hvvm_mbox_valid,
	.set_hvvm_mbox_ack = gpuiov_v9_0_set_hvvm_mbox_ack,
	.get_hvvm_mbox_msg_valid = gpuiov_v9_0_get_hvvm_mbox_msg_valid,
	.clear_intr_status = gpuiov_v9_0_clear_intr_status,
	.get_active_vfs = gpuiov_v9_0_get_active_vfs,
	.set_active_vfs = gpuiov_v9_0_set_active_vfs,
	.add_active_vf = gpuiov_v9_0_add_active_vf,
	.remove_active_vf = gpuiov_v9_0_remove_active_vf,
	.get_active_vf_idx = gpuiov_v9_0_get_active_vf_idx,
	.get_active_vf_status = gpuiov_v9_0_get_active_vf_status,
	.get_time_quanta_index = gpuiov_v9_0_get_time_quanta_index,
	.set_time_quanta_index = gpuiov_v9_0_set_time_quanta_index,
	.get_time_quanta_definition = gpuiov_v9_0_get_time_quanta_definition,
	.set_time_quanta_definition = gpuiov_v9_0_set_time_quanta_definition,
	.get_time_quanta_option = gpuiov_v9_0_get_time_quanta_option,
	.set_time_quanta_option = gpuiov_v9_0_set_time_quanta_option,
	.set_vf_access = gpuiov_v9_0_set_vf_access,
	.get_vf_access = gpuiov_v9_0_get_vf_access,
	.get_config_info = gpuiov_v9_0_get_config_info,
	.wait_auto_sched_stop = gpuiov_v9_0_wait_auto_sched_stop,
	.skip_ctrl_block = gpuiov_v9_0_skip_ctrl_block,
};

static int gpuiov_v9_0_sw_init(struct amdgv_adapter *adapt)
{
	adapt->gpuiov.funcs = &gpuiov_v9_0_funcs;

	adapt->gpuiov.lock =  oss_mutex_init();

	return amdgv_gpuiov_ctrl_block_setup(adapt, gpuiov_v9_0_static_config,
					     ARRAY_SIZE(gpuiov_v9_0_static_config));
}

static int gpuiov_v9_0_sw_fini(struct amdgv_adapter *adapt)
{
	oss_mutex_fini(adapt->gpuiov.lock);

	return 0;
}

static int gpuiov_v9_0_hw_fini(struct amdgv_adapter *adapt)
{
	if (oss_atomic_read(adapt->in_sync_flood)) {
		if (adapt->vf_rebar_en)
			gpuiov_v9_0_toggle_vf_mse(adapt, false);
		return 0;
	}

	/* disable sriov */
	if (!in_whole_gpu_reset()) {
		oss_pci_disable_sriov(adapt->dev);

		amdgv_gpuiov_fini(adapt);
	} else {
		oss_pci_write_config_dword(adapt->dev,
					   adapt->sriov_cap_pos + PCIE_EXT_SRIOV_CTRL, 0);
	}

	return 0;
}

static int gpuiov_v9_0_hw_init(struct amdgv_adapter *adapt)
{
	int ret;
	uint32_t xgmi_enable;
	uint32_t cap;
	uint16_t tmp;

	if (oss_atomic_read(adapt->in_ecc_recovery)) {
		gpuiov_v9_0_hw_fini(adapt);
		oss_msleep(500);
	}

	adapt->gpuiov.pos = gpuiov_v9_0_find_cap(adapt);
	if (adapt->gpuiov.pos == 0) {
		return AMDGV_FAILURE;
	}

	if (amdgv_gpuiov_init(adapt) < 0)
		return AMDGV_FAILURE;

	if (gpuiov_v9_0_get_fb_info(adapt))
		return AMDGV_FAILURE;

	gpuiov_v9_0_get_sch_offset(adapt);

	if (adapt->xgmi.phy_nodes_num > 1) {
		if (adapt->psp.program_register) {
			xgmi_enable = 0x1 | (1 << 31);
			adapt->psp.program_register(adapt, AMDGV_PF_IDX, xgmi_enable,
				0, GC_MC_VM_XGMI_GPUIOV_ENABLE);
			adapt->psp.program_register(adapt, AMDGV_PF_IDX, xgmi_enable,
				0, MM_MC_VM_XGMI_GPUIOV_ENABLE);
		}
	}

	/* Need to check if autoload is complete before interacting with rlc xt firmware */
	ret = amdgv_gfx_check_rlc_autoload_complete(adapt);
	if (ret)
		return ret;

	/* enable sriov */
	if (!in_whole_gpu_reset()) {
		ret = oss_pci_enable_sriov(adapt->dev, adapt->num_vf);
		if (ret < 0) {
			amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_IOV_ENABLE_SRIOV_FAIL, 0);
			return AMDGV_FAILURE;
		}
		AMDGV_INFO("PCI_ENABLE_SRIOV(num_vf=%d)\n", adapt->num_vf);

		oss_pci_read_config_dword(adapt->dev, adapt->sriov_cap_pos + PCIE_EXT_SRIOV_CAP, &cap);
		if (cap & PCIE_EXT_SRIOV_CAP_VF_10BIT_TAG) {
			oss_pci_read_config_word(adapt->dev, adapt->sriov_cap_pos + PCIE_EXT_SRIOV_CTRL, &tmp);
			tmp |= PCIE_EXT_SRIOV_CTRL_VF_10BIT_TAG;
			oss_pci_write_config_word(adapt->dev,
				adapt->sriov_cap_pos + PCIE_EXT_SRIOV_CTRL, tmp);
		}
	} else {
		amdgv_reset_restore_sriov(adapt);
	}

	return 0;
}

struct amdgv_init_func gpuiov_v9_0_func = {
	.name = "gpuiov_v9_0_func",
	.sw_init = gpuiov_v9_0_sw_init,
	.sw_fini = gpuiov_v9_0_sw_fini,
	.hw_init = gpuiov_v9_0_hw_init,
	.hw_fini = gpuiov_v9_0_hw_fini,
};
