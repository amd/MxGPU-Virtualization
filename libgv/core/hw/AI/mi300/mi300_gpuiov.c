/*
 * Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
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
#include <amdgv_gpuiov.h>
#include <amdgv_psp.h>
#include <amdgv.h>
#include "mi300.h"
#include "mi300_gpuiov.h"
#include "mi300_nbio.h"
#include "mmhub_v1_8.h"
#include "gfxhub_v1_2.h"

#include "mi300_vcn.h"

static const int this_block = AMDGV_COMMUNICATION_BLOCK;

/* the size is in units of 256K */
#define CSA_SIZE_PER_VF 1
#define UNIT_256KB	(1 << 18)
#define MI300_MAX_XCD_NUM	8
#define MI300_SUPPORTED_GFX_SCHED_MODE ((1 << AMDGV_SCHED_FAIRNESS) | (1 << AMDGV_SCHED_ROUND_ROBIN))

static struct amdgv_gpuiov_hw_sched_static_config mi300_hw_sched_static_config[] = {

	{"VCN_SCH0_MMSCH", 		AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_VCN, 	AMDGV_SCHED_FRAME_LOOP_MODE, 	PCI_GPUIOV_VCN_SCH0_OFFSET, 0, 1 << AMDGV_SCHED_FRAME_LOOP_MODE},
	{"JPEG_SCH0_MMSCH", 	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_JPEG, AMDGV_SCHED_FRAME_LOOP_MODE, 	PCI_GPUIOV_JPEG_SCH0_OFFSET, 0, 1 << AMDGV_SCHED_FRAME_LOOP_MODE},
	{"JPEG1_SCH0_MMSCH", 	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_JPEG, AMDGV_SCHED_FRAME_LOOP_MODE, 	PCI_GPUIOV_JPEG1_SCH0_OFFSET, 0, 1 << AMDGV_SCHED_FRAME_LOOP_MODE},
	{"VCN_SCH1_MMSCH", 		AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_VCN, 	AMDGV_SCHED_FRAME_LOOP_MODE, 	PCI_GPUIOV_VCN_SCH1_OFFSET, 1, 1 << AMDGV_SCHED_FRAME_LOOP_MODE},
	{"JPEG_SCH1_MMSCH", 	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_JPEG, AMDGV_SCHED_FRAME_LOOP_MODE, 	PCI_GPUIOV_JPEG_SCH1_OFFSET, 1, 1 << AMDGV_SCHED_FRAME_LOOP_MODE},
	{"JPEG1_SCH1_MMSCH", 	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_JPEG, AMDGV_SCHED_FRAME_LOOP_MODE, 	PCI_GPUIOV_JPEG1_SCH1_OFFSET, 1, 1 << AMDGV_SCHED_FRAME_LOOP_MODE},
	{"VCN_SCH2_MMSCH", 		AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_VCN, 	AMDGV_SCHED_FRAME_LOOP_MODE, 	PCI_GPUIOV_VCN_SCH2_OFFSET, 2, 1 << AMDGV_SCHED_FRAME_LOOP_MODE},
	{"JPEG_SCH2_MMSCH", 	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_JPEG, AMDGV_SCHED_FRAME_LOOP_MODE, 	PCI_GPUIOV_JPEG_SCH2_OFFSET, 2, 1 << AMDGV_SCHED_FRAME_LOOP_MODE},
	{"JPEG1_SCH2_MMSCH", 	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_JPEG, AMDGV_SCHED_FRAME_LOOP_MODE, 	PCI_GPUIOV_JPEG1_SCH2_OFFSET, 2, 1 << AMDGV_SCHED_FRAME_LOOP_MODE},
	{"VCN_SCH3_MMSCH", 		AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_VCN, 	AMDGV_SCHED_FRAME_LOOP_MODE, 	PCI_GPUIOV_VCN_SCH3_OFFSET, 3, 1 << AMDGV_SCHED_FRAME_LOOP_MODE},
	{"JPEG_SCH3_MMSCH", 	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_JPEG, AMDGV_SCHED_FRAME_LOOP_MODE, 	PCI_GPUIOV_JPEG_SCH3_OFFSET, 3, 1 << AMDGV_SCHED_FRAME_LOOP_MODE},
	{"JPEG1_SCH3_MMSCH", 	AMDGV_HW_SCHED_TYPE_MM,		AMDGV_SCHED_BLOCK_JPEG, AMDGV_SCHED_FRAME_LOOP_MODE, 	PCI_GPUIOV_JPEG1_SCH3_OFFSET, 3, 1 << AMDGV_SCHED_FRAME_LOOP_MODE},
	{"GFX_SCH0_RLCV",		AMDGV_HW_SCHED_TYPE_GFX,	AMDGV_SCHED_BLOCK_GFX, 	AMDGV_SCHED_FAIRNESS, 			PCI_GPUIOV_GFX_SCH0_OFFSET, 0, MI300_SUPPORTED_GFX_SCHED_MODE},
	{"GFX_SCH1_RLCV",		AMDGV_HW_SCHED_TYPE_GFX,	AMDGV_SCHED_BLOCK_GFX, 	AMDGV_SCHED_FAIRNESS, 			PCI_GPUIOV_GFX_SCH1_OFFSET, 1, MI300_SUPPORTED_GFX_SCHED_MODE},
	{"GFX_SCH2_RLCV",		AMDGV_HW_SCHED_TYPE_GFX,	AMDGV_SCHED_BLOCK_GFX, 	AMDGV_SCHED_FAIRNESS, 			PCI_GPUIOV_GFX_SCH2_OFFSET, 2, MI300_SUPPORTED_GFX_SCHED_MODE},
	{"GFX_SCH3_RLCV",		AMDGV_HW_SCHED_TYPE_GFX,	AMDGV_SCHED_BLOCK_GFX, 	AMDGV_SCHED_FAIRNESS, 			PCI_GPUIOV_GFX_SCH3_OFFSET, 3, MI300_SUPPORTED_GFX_SCHED_MODE},
	{"GFX_SCH4_RLCV",		AMDGV_HW_SCHED_TYPE_GFX,	AMDGV_SCHED_BLOCK_GFX, 	AMDGV_SCHED_FAIRNESS, 			PCI_GPUIOV_GFX_SCH4_OFFSET, 4, MI300_SUPPORTED_GFX_SCHED_MODE},
	{"GFX_SCH5_RLCV",		AMDGV_HW_SCHED_TYPE_GFX,	AMDGV_SCHED_BLOCK_GFX, 	AMDGV_SCHED_FAIRNESS, 			PCI_GPUIOV_GFX_SCH5_OFFSET, 5, MI300_SUPPORTED_GFX_SCHED_MODE},
	{"GFX_SCH6_RLCV",		AMDGV_HW_SCHED_TYPE_GFX,	AMDGV_SCHED_BLOCK_GFX, 	AMDGV_SCHED_FAIRNESS, 			PCI_GPUIOV_GFX_SCH6_OFFSET, 6, MI300_SUPPORTED_GFX_SCHED_MODE},
	{"GFX_SCH7_RLCV",		AMDGV_HW_SCHED_TYPE_GFX,	AMDGV_SCHED_BLOCK_GFX, 	AMDGV_SCHED_FAIRNESS, 			PCI_GPUIOV_GFX_SCH7_OFFSET, 7, MI300_SUPPORTED_GFX_SCHED_MODE},
};

static int mi300_gpuiov_find_cap(struct amdgv_adapter *adapt)
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
		AMDGV_ERROR("No GPUIOV caps can be found.\n");
		return 0;
	}

	return pos;
}

static int mi300_gpuiov_get_sched_block_offset(struct amdgv_adapter *adapt,
					       uint32_t hw_sched_id)
{

	if (hw_sched_id >= adapt->gpuiov.num_ctrl_blocks) {
		AMDGV_ERROR("%s(%d) is an invalid scheduler for this operation\n",
			amdgv_hw_sched_id_to_name(adapt, hw_sched_id), hw_sched_id);
		return AMDGV_FAILURE;
	}

	return (adapt->gpuiov.pos + (adapt->gpuiov.ctrl_blocks[hw_sched_id].offset << 4));
}

static int mi300_gpuiov_set_cmd(struct amdgv_adapter *adapt,
				 enum amdgv_gpuiov_cmd cmd,
				 uint32_t hw_sched_id,
				 uint32_t idx_vf, uint32_t next_idx_vf)
{
	uint32_t data, func_id, next_func_id;
	int offset;
	uint64_t reg_control = 0;

	func_id = PCI_GPUIOV_FUNC_ID(idx_vf);

	next_func_id = PCI_GPUIOV_FUNC_ID(next_idx_vf);

	offset = mi300_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE) {
		AMDGV_ERROR("Get wrong offset\n");
		return AMDGV_FAILURE;
	}

	offset += PCI_SCH_CMD_CONTROL;

	data = (cmd & 0x0F) | CMD_EXECUTE | (func_id << 8) | (next_func_id << 16);

	if (!oss_atomic_read(adapt->in_sync_flood)) {
		if (IS_HW_SCHED_TYPE_MM(hw_sched_id)) {
			mi300_vcn_get_mmsch_regid_instid(adapt, hw_sched_id, &reg_control, true);
			WREG32(reg_control, data);
		} else
			oss_pci_write_config_dword(adapt->dev, offset, data);
	} else {
		AMDGV_DEBUG("In in_sync_flood. Skip idx_vf=%d sched_id=%d (%s) cmd=0x%x (%s) func_id=0x%x "
									"next_func_id=0x%x pci_write_config_dword(0x%08x, 0x%08x)\n",
									idx_vf, hw_sched_id, amdgv_hw_sched_id_to_name(adapt, hw_sched_id), cmd,
									amdgv_gpuiov_cmd_to_name(adapt, cmd, hw_sched_id), func_id, next_func_id,
									offset, data);
	}

	adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd = cmd;
	adapt->gpuiov.ctrl_blocks[hw_sched_id].last_status = AMDGV_CMD_STATUS_PENDING_EXECUTE;
	return 0;
}

static bool mi300_gpuiov_is_cmd_complete(struct amdgv_adapter *adapt,
					uint32_t hw_sched_id)
{
	uint8_t command, status = 0;
	int cmd_ctrl_offset, cmd_status_offset;
	int offset;
	uint64_t reg_control, reg_status;
	reg_control = 0;
	reg_status = 0;

	if (IS_HW_SCHED_TYPE_MM(hw_sched_id)) {
		mi300_vcn_get_mmsch_regid_instid(adapt, hw_sched_id, &reg_control, true);
		mi300_vcn_get_mmsch_regid_instid(adapt, hw_sched_id, &reg_status, false);

		command = RREG32(reg_control);
		status = RREG32(reg_status) & 0xFF;
	} else {
		offset = mi300_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
		if (offset == AMDGV_FAILURE) {
			AMDGV_ERROR("Get wrong offset\n");
			return AMDGV_FAILURE;
		}
		cmd_ctrl_offset = PCI_SCH_CMD_CONTROL + offset;
		cmd_status_offset = PCI_SCH_CMD_STATUS + offset;

		oss_pci_read_config_byte(adapt->dev, cmd_ctrl_offset, &command);
		oss_pci_read_config_byte(adapt->dev, cmd_status_offset, &status);
	}
	adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd = command;
	adapt->gpuiov.ctrl_blocks[hw_sched_id].last_status = status;

	return ((!(command & CMD_EXECUTE)) && status == 0);
}

static void mi300_dump_gpuiov_cmd_status(struct amdgv_adapter *adapt, uint32_t hw_sched_id)
{
	uint8_t command, status;

	command = adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd;
	status = adapt->gpuiov.ctrl_blocks[hw_sched_id].last_status;

	AMDGV_INFO("hw_sched_id=%d (%s) GPUIOV_CMD=0x%x (%s) CMD_STATUS=0x%x (%s)\n", hw_sched_id,
		    amdgv_hw_sched_id_to_name(adapt, hw_sched_id), command,
		    amdgv_gpuiov_cmd_to_name(adapt, command, hw_sched_id), status,
		    amdgv_gpuiov_status_to_name(status));
}

static void mi300_gpuiov_set_csa(struct amdgv_adapter *adapt, uint64_t csa_base)
{
	uint32_t data, offset;
	uint32_t cntx_size, cntx_loc, cntx_offset;
	int i;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_CNTXT;

	AMDGV_DEBUG("CSA address = 0x%llx\n", csa_base);
	data = PCI_GPUIOV_CNTXT__SIZE__PUT(CSA_SIZE_PER_VF) |
	       PCI_GPUIOV_CNTXT__LOC__PUT(PCI_GPUIOV_CNTXT__LOC_IN_FB) |
	       PCI_GPUIOV_CNTXT__OFFSET__PUT(
	       TO_256KBYTES(csa_base));

	//clear CSA
	oss_memset(amdgv_memmgr_get_cpu_addr(adapt->gpuiov.csa_fb_mem),
			0, amdgv_memmgr_get_size(adapt->gpuiov.csa_fb_mem));

	if (adapt->dev_id == 0x74A0) {
		for (i = 0; i < adapt->mcp.gfx.num_xcc; i++) {
			WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, i), regRLC_GPU_IOV_CFG_REG6), data);
			data = RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, i), regRLC_GPU_IOV_CFG_REG6));
			cntx_size = PCI_GPUIOV_CNTXT__SIZE(data);
			cntx_loc = PCI_GPUIOV_CNTXT__LOC(data);
			cntx_offset = PCI_GPUIOV_CNTXT__OFFSET(data);

			AMDGV_DEBUG("RLC_GPU_IOV_CFG_REG6(%d)=0x%08x (readback) "
				"CONTEXT_OFFSET[31:10]=0x%x LOC=0x%x CONTEXT_SIZE[6:0]=0x%x\n",
				i, data, cntx_offset, cntx_loc, cntx_size);
			}
	} else {
		oss_pci_write_config_dword(adapt->dev, offset, data);
		oss_pci_read_config_dword(adapt->dev, offset, &data);
		cntx_size = PCI_GPUIOV_CNTXT__SIZE(data);
		cntx_loc = PCI_GPUIOV_CNTXT__LOC(data);
		cntx_offset = PCI_GPUIOV_CNTXT__OFFSET(data);

		AMDGV_DEBUG("PCI_GPUIOV_CNTXT=0x%08x (readback) "
				"CONTEXT_OFFSET[31:10]=0x%x LOC=0x%x CONTEXT_SIZE[6:0]=0x%x\n",
				data, cntx_offset, cntx_loc, cntx_size);
	}
}

static int mi300_gpuiov_set_total_fb_consumed(struct amdgv_adapter *adapt,
					      uint16_t total_fb_consumed)
{
	uint32_t offset = adapt->gpuiov.pos + PCI_GPUIOV_TOTAL_FB_CONSUMED;

	return oss_pci_write_config_word(adapt->dev, offset, total_fb_consumed);
}

static int mi300_gpuiov_set_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf,
				  uint32_t fb_offset, uint32_t fb_size)
{
	uint32_t data = ((fb_size >> 4)) | ((fb_offset >> 4) << 16);
	uint32_t offset =
		adapt->gpuiov.pos + PCI_GPUIOV_VF0_FB_SIZE + idx_vf * sizeof(uint32_t);

	AMDGV_INFO("idx_vf = 0x%x, fb_offset = %d MB, fb_size = %d MB\n", idx_vf, fb_offset,
		   fb_size);

	adapt->array_vf[idx_vf].real_fb_size = fb_size;
	return oss_pci_write_config_dword(adapt->dev, offset, data);
}

static int mi300_gpuiov_get_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf,
				  uint32_t *fb_offset, uint32_t *fb_size, uint32_t *real_fb_size)
{
	uint32_t data;
	uint32_t offset =
		adapt->gpuiov.pos + PCI_GPUIOV_VF0_FB_SIZE + idx_vf * sizeof(uint32_t);

	if (oss_pci_read_config_dword(adapt->dev, offset, &data)) {
		AMDGV_ERROR("Cannot read %s fb from PCIe config\n",
			amdgv_idx_to_str(idx_vf));
		return AMDGV_FAILURE;
	}

	*fb_offset = ((data & 0xFFFF0000) >> 16) << 4;
	*real_fb_size = (data & 0x0000FFFF) << 4;
	*fb_size = *real_fb_size;

	return 0;
}

static int mi300_gpuiov_get_vm_busy_status(struct amdgv_adapter *adapt,
					   uint32_t hw_sched_id,
					   uint32_t *vm_busy_status)
{
	int offset = mi300_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE) {
		AMDGV_ERROR("Get wrong offset\n");
		return AMDGV_FAILURE;
	}

	offset += PCI_SCH_VM_BUSY_STATUS;

	return oss_pci_read_config_dword(adapt->dev, offset, vm_busy_status);
}

static int mi300_gpuiov_clear_intr_status(struct amdgv_adapter *adapt, uint32_t bits)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_INTR_STATUS;

	return oss_pci_write_config_dword(adapt->dev, offset, bits);
}

static int mi300_gpuiov_get_intr_bits(struct amdgv_adapter *adapt, uint32_t *bits)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_INTR_ENABLE;
	return oss_pci_read_config_dword(adapt->dev, offset, bits);
}

static int mi300_gpuiov_set_intr_bits(struct amdgv_adapter *adapt, uint32_t bits)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_INTR_ENABLE;

	return oss_pci_write_config_dword(adapt->dev, offset, bits);
}

static int mi300_gpuiov_get_intr_status(struct amdgv_adapter *adapt, uint32_t *bits)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_INTR_STATUS;

	return oss_pci_read_config_dword(adapt->dev, offset, bits);
}

static int mi300_gpuiov_get_hvvm_mbox_index(struct amdgv_adapter *adapt, uint8_t *index)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_HVVM_MBOX0;

	return oss_pci_read_config_byte(adapt->dev, offset, index);
}

static int mi300_gpuiov_update_hvvm_mbox_index(struct amdgv_adapter *adapt, uint8_t index)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_HVVM_MBOX0;

	return oss_pci_write_config_byte(adapt->dev, offset, index);
}

int mi300_gpuiov_rcv_hvvm_mbox_msg(struct amdgv_adapter *adapt, uint8_t *msg_data)
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

int mi300_gpuiov_trn_hvvm_mbox_data(struct amdgv_adapter *adapt, uint8_t msg_data)
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

int mi300_gpuiov_set_hvvm_mbox_valid(struct amdgv_adapter *adapt, uint8_t bits)
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

int mi300_gpuiov_set_hvvm_mbox_ack(struct amdgv_adapter *adapt)
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

static int mi300_gpuiov_get_hvvm_mbox_msg_valid(struct amdgv_adapter *adapt, uint32_t *bit_map)
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

static int mi300_gpuiov_get_active_vfs(struct amdgv_adapter *adapt,
				       uint32_t hw_sched_id,
				       uint32_t *active_vfs)
{
	int offset;

	offset = mi300_gpuiov_get_sched_block_offset(adapt, hw_sched_id) +
		 PCI_SCH_ACTIVE_FUNCTIONS;
	oss_pci_read_config_dword(adapt->dev, offset, active_vfs);

	return 0;
}

static int mi300_gpuiov_set_active_vfs(struct amdgv_adapter *adapt,
				       uint32_t hw_sched_id,
				       uint32_t active_vfs)
{
	int offset;

	offset = mi300_gpuiov_get_sched_block_offset(adapt, hw_sched_id) +
		 PCI_SCH_ACTIVE_FUNCTIONS;
	oss_pci_write_config_dword(adapt->dev, offset, active_vfs);

	return 0;
}

static int mi300_gpuiov_remove_active_vf(struct amdgv_adapter *adapt,
					 uint32_t hw_sched_id,
					 uint32_t idx_vf)
{
	uint32_t active_vfs = 0;

	mi300_gpuiov_get_active_vfs(adapt, hw_sched_id, &active_vfs);

	if (idx_vf == AMDGV_PF_IDX)
		active_vfs &= ~(1U << 31);
	else
		active_vfs &= ~(1U << idx_vf);

	mi300_gpuiov_set_active_vfs(adapt, hw_sched_id, active_vfs);
	return 0;
}

static int mi300_gpuiov_get_active_vf_idx(struct amdgv_adapter *adapt,
					  uint32_t hw_sched_id,
					  uint32_t *idx_vf)
{
	int offset;
	uint32_t data;

	offset = mi300_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE) {
		AMDGV_ERROR("wrong offset for hw_sched_id=%d\n", hw_sched_id);
		return AMDGV_FAILURE;
	}

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

static int mi300_gpuiov_get_active_vf_status(struct amdgv_adapter *adapt,
					     uint32_t hw_sched_id,
					     uint8_t *status)
{
	int offset;

	offset = mi300_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE) {
		AMDGV_ERROR("Get wrong offset\n");
		return AMDGV_FAILURE;
	}

	offset += PCI_SCH_ACTIVE_FUNCTION_ID_STATUS;
	oss_pci_read_config_byte(adapt->dev, offset, status);

	return 0;
}

static int mi300_gpuiov_get_time_quanta_index(struct amdgv_adapter *adapt,
					      uint32_t idx_vf,
					      uint32_t hw_sched_id,
					      uint32_t *time_quanta_index)
{
	uint32_t data;
	int offset;

	offset = mi300_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE) {
		AMDGV_ERROR("Get wrong offset\n");
		return AMDGV_FAILURE;
	}

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

static int mi300_gpuiov_set_time_quanta_index(struct amdgv_adapter *adapt,
					      uint32_t idx_vf,
					      uint32_t hw_sched_id,
					      uint32_t time_quanta_index)
{
	uint32_t data;
	int offset;

	offset = mi300_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE) {
		AMDGV_ERROR("Get wrong offset\n");
		return AMDGV_FAILURE;
	}

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

static int mi300_gpuiov_get_time_quanta_definition(struct amdgv_adapter *adapt,
					uint32_t hw_sched_id,
					int index, uint8_t *quanta_option)
{
	uint32_t data;
	int offset;

	offset = mi300_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE) {
		AMDGV_ERROR("Get wrong offset\n");
		return AMDGV_FAILURE;
	}

	offset += PCI_SCH_TIME_QUANTA_OPTION;
	oss_pci_read_config_dword(adapt->dev, offset, &data);
	*quanta_option = (data >> (index * 8)) & 0xff;

	return 0;
}

static int mi300_gpuiov_set_time_quanta_definition(struct amdgv_adapter *adapt,
					uint32_t hw_sched_id,
					int index, uint8_t quanta_option)
{
	uint32_t data;
	int offset;

	offset = mi300_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE) {
		AMDGV_ERROR("Get wrong offset\n");
		return AMDGV_FAILURE;
	}

	offset += PCI_SCH_TIME_QUANTA_OPTION;
	oss_pci_read_config_dword(adapt->dev, offset, &data);

	data &= ~(0xff << (index * 8));
	data |= quanta_option << (index * 8);

	oss_pci_write_config_dword(adapt->dev, offset, data);
	return 0;
}

static int mi300_gpuiov_add_active_vf(struct amdgv_adapter *adapt,
				      uint32_t hw_sched_id,
				      uint32_t idx_vf)
{
	uint32_t active_vfs = 0;

	mi300_gpuiov_get_active_vfs(adapt, hw_sched_id, &active_vfs);

	if (idx_vf == AMDGV_PF_IDX)
		active_vfs |= (1U << 31);
	else
		active_vfs |= (1U << idx_vf);

	mi300_gpuiov_set_active_vfs(adapt, hw_sched_id, active_vfs);
	return 0;
}

static int mi300_gpuiov_get_time_quanta_option(struct amdgv_adapter *adapt,
					    uint32_t hw_sched_id,
					    uint32_t *time_quanta_option)
{
	int offset;

	offset = mi300_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE) {
		AMDGV_ERROR("Get wrong offset\n");
		return AMDGV_FAILURE;
	}

	offset += PCI_SCH_TIME_QUANTA_OPTION;
	oss_pci_read_config_dword(adapt->dev, offset, time_quanta_option);
	return 0;
}

static int mi300_gpuiov_set_time_quanta_option(struct amdgv_adapter *adapt,
					    uint32_t hw_sched_id,
					    uint32_t time_quanta_option)
{
	int offset;

	offset = mi300_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE) {
		AMDGV_ERROR("Get wrong offset\n");
		return AMDGV_FAILURE;
	}

	offset += PCI_SCH_TIME_QUANTA_OPTION;
	oss_pci_write_config_dword(adapt->dev, offset, time_quanta_option);

	return 0;
}

static int mi300_gpuiov_set_vf_access(struct amdgv_adapter *adapt,
				      uint32_t idx_vf,
				      uint32_t vf_access_select, bool is_true)
{
	uint32_t tmp, value, origin;

	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;
	if (idx_vf == AMDGV_PF_IDX)
		return 0;
	if (adapt->flags & AMDGV_FLAG_DISABLE_MMIO_PROTECTION) {
		AMDGV_DEBUG("force disable vf mmio protection\n");
		vf_access_select = AMDGV_VF_ACCESS_ALL;
		is_true = true;
	}

	value = (uint32_t)0x1 << idx_vf;

	if (vf_access_select & AMDGV_VF_ACCESS_FB) {
		tmp = RREG32(mmnbif_gpu_VF_FB_EN);
		origin = tmp;
		if (is_true)
			tmp |= value;
		else
			tmp &= ~value;
		if (origin != tmp)
			WREG32(mmnbif_gpu_VF_FB_EN, tmp);
	}

	if (vf_access_select & AMDGV_VF_ACCESS_DOORBELL) {
		tmp = RREG32(mmnbif_gpu_VF_DOORBELL_EN);
		origin = tmp;
		if (is_true)
			tmp |= value;
		else
			tmp &= ~value;
		if (origin != tmp)
			WREG32(mmnbif_gpu_VF_DOORBELL_EN, tmp);
	}

	if (vf_access_select & AMDGV_VF_ACCESS_MMIO_REG_WRITE) {
		tmp = RREG32(mmnbif_gpu_VF_REGWR_EN);
		origin = tmp;
		if (is_true)
			tmp |= value;
		else
			tmp &= ~value;
		if (origin != tmp)
			WREG32(mmnbif_gpu_VF_REGWR_EN, tmp);
	}

	return 0;
}

static bool mi300_gpuiov_get_vf_access(struct amdgv_adapter *adapt, uint32_t idx_vf,
				      uint32_t vf_access_select)
{
	uint32_t tmp, value;

	if (idx_vf == AMDGV_PF_IDX)
		return false;
	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return false;

	value = (uint32_t)0x1 << idx_vf;

	if (vf_access_select & AMDGV_VF_ACCESS_FB)
		tmp = RREG32(mmnbif_gpu_VF_FB_EN);
	else if (vf_access_select & AMDGV_VF_ACCESS_DOORBELL)
		tmp = RREG32(mmnbif_gpu_VF_DOORBELL_EN);
	else if (vf_access_select & AMDGV_VF_ACCESS_MMIO_REG_WRITE)
		tmp = RREG32(mmnbif_gpu_VF_REGWR_EN);
	else
		return false;

	return tmp & value;
}

static int mi300_gpuiov_get_fb_info(struct amdgv_adapter *adapt)
{
	/* read total available framebuffer */
	adapt->gpuiov.total_fb_avail = mi300_nbio_get_config_memsize(adapt);
	adapt->gpuiov.total_fb_usable = adapt->gpuiov.total_fb_avail - amdgv_vbios_get_fw_reserved_size(adapt);

	AMDGV_INFO(
		"Total FB Available = %d MB, Max usable FB size = %d MB\n",
		adapt->gpuiov.total_fb_avail, adapt->gpuiov.total_fb_usable);

	/* compute CSA address (offset) to be sent to RLC_V and MMSCH */
	if (!adapt->gpuiov.csa_fb_mem) {
		AMDGV_ERROR("Private csa fb memory not allocated\n");
		return AMDGV_FAILURE;
	}

	adapt->gpuiov.resv_addr = amdgv_memmgr_get_offset(adapt->gpuiov.csa_fb_mem);
	adapt->gpuiov.resv_size = amdgv_memmgr_get_size(adapt->gpuiov.csa_fb_mem);

	return 0;
}

static bool mi300_gpuiov_skip_reading_sch_offset(struct amdgv_adapter *adapt, uint32_t hw_sched_id)
{
	//@TODO: Check for MMSCH as well?

	if (IS_HW_SCHED_TYPE_GFX(hw_sched_id))
		return !(adapt->mcp.gfx.xcc_mask & (1 << adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_inst));

	return false;
}

static void mi300_gpuiov_get_sch_offset(struct amdgv_adapter *adapt)
{
	uint32_t offset, i;

	for (i = 0; i < adapt->gpuiov.num_ctrl_blocks; i++) {
		if (mi300_gpuiov_skip_reading_sch_offset(adapt, i))
			continue;

		offset = adapt->gpuiov.pos + adapt->gpuiov.ctrl_blocks[i].pci_gpuiov_offset;
		oss_pci_read_config_byte(adapt->dev, offset, &adapt->gpuiov.ctrl_blocks[i].offset);

		AMDGV_DEBUG("pci offset = 0x%x %s block offset = 0x%x\n", offset,\
			adapt->gpuiov.ctrl_blocks[i].name, 		\
			adapt->gpuiov.ctrl_blocks[i].offset << 4);
	}
}

static int mi300_gpuiov_get_config_info(struct amdgv_adapter *adapt)
{
	adapt->gpuiov.pos = mi300_gpuiov_find_cap(adapt);
	AMDGV_DEBUG("pci cap offset = 0x%x\n", adapt->gpuiov.pos);

	if (adapt->gpuiov.pos == 0)
		return AMDGV_FAILURE;

	if (mi300_gpuiov_get_fb_info(adapt))
		return AMDGV_FAILURE;

	mi300_gpuiov_get_sch_offset(adapt);

	if (oss_pci_find_ext_cap(adapt->dev, PCIE_EXT_CAP_ID__REBAR) != 0)
		adapt->vf_rebar_en = true;

	return 0;
}

static int mi300_gpuiov_wait_auto_sched_stop(struct amdgv_adapter *adapt,
						uint32_t hw_sched_id)
{
	struct amdgv_sched_world_switch *world_switch;
	int offset;
	int wait_ret;

	if (amdgv_sched_get_world_switch_by_hw_sched_id(adapt, hw_sched_id, &world_switch))
		return AMDGV_FAILURE;

	if (!world_switch->enabled)
		return AMDGV_FAILURE;

	offset = mi300_gpuiov_get_sched_block_offset(adapt, hw_sched_id);

	if (offset == AMDGV_FAILURE) {
		AMDGV_ERROR("Cannot find offset for %s scheduler in PCIe config\n",
			amdgv_hw_sched_id_to_name(adapt, hw_sched_id));
		return AMDGV_FAILURE;
	}

	offset += PCI_SCH_ACTIVE_FUNCTION_ID_STATUS;
	wait_ret = amdgv_wait_for_pci_cfg(adapt, adapt->dev, offset,
			0xf, 0, 1, AMDGV_TIMEOUT(TIMEOUT_AUTO_SWITCH_MM), AMDGV_WAIT_CHECK_EQ, 0);

	if (wait_ret)
		AMDGV_WARN("Fail to wait auto sched %s scheduler stop\n",
			amdgv_hw_sched_id_to_name(adapt, hw_sched_id));

	return wait_ret;
}

static int mi300_gpuiov_toggle_rlcg_vf_interface(struct amdgv_adapter *adapt, uint32_t idx_vf, bool enable)
{
	uint32_t xcc_id;

	AMDGV_INFO("RLCG VF Interface is %s\n", enable ? "enabled" : "disabled");
	for_each_id (xcc_id, amdgv_sched_get_xcc_mask_by_vf(adapt, idx_vf)) {
		WREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regRLC_GPM_GENERAL_14), enable ? 1 : 0);
	}

	return 0;
}

static bool mi300_gpuiov_skip_ctrl_block(struct amdgv_adapter *adapt, int idx)
{
	return (mi300_hw_sched_static_config[idx].hw_sched_type == AMDGV_HW_SCHED_TYPE_GFX &&
			!(adapt->mcp.gfx.xcc_mask & (1 << mi300_hw_sched_static_config[idx].hw_inst)));
}

static void mi300_gpuiov_toggle_vf_mse(struct amdgv_adapter *adapt, bool enable)
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

static const struct amdgv_gpuiov_funcs mi300_gpuiov_funcs = {
	.set_cmd = mi300_gpuiov_set_cmd,
	.is_cmd_complete = mi300_gpuiov_is_cmd_complete,
	.dump_gpuiov_cmd_status = mi300_dump_gpuiov_cmd_status,
	.set_total_fb_consumed = mi300_gpuiov_set_total_fb_consumed,
	.set_vf_fb = mi300_gpuiov_set_vf_fb,
	.get_vf_fb = mi300_gpuiov_get_vf_fb,
	.get_vm_busy_status = mi300_gpuiov_get_vm_busy_status,
	.get_intr_bits = mi300_gpuiov_get_intr_bits,
	.set_intr_bits = mi300_gpuiov_set_intr_bits,
	.get_intr_status = mi300_gpuiov_get_intr_status,
	.get_hvvm_mbox_index = mi300_gpuiov_get_hvvm_mbox_index,
	.update_hvvm_mbox_index = mi300_gpuiov_update_hvvm_mbox_index,
	.rcv_hvvm_mbox_msg = mi300_gpuiov_rcv_hvvm_mbox_msg,
	.trn_hvvm_mbox_data = mi300_gpuiov_trn_hvvm_mbox_data,
	.set_hvvm_mbox_valid = mi300_gpuiov_set_hvvm_mbox_valid,
	.set_hvvm_mbox_ack = mi300_gpuiov_set_hvvm_mbox_ack,
	.get_hvvm_mbox_msg_valid = mi300_gpuiov_get_hvvm_mbox_msg_valid,
	.clear_intr_status = mi300_gpuiov_clear_intr_status,
	.get_active_vfs = mi300_gpuiov_get_active_vfs,
	.set_active_vfs = mi300_gpuiov_set_active_vfs,
	.add_active_vf = mi300_gpuiov_add_active_vf,
	.remove_active_vf = mi300_gpuiov_remove_active_vf,
	.get_active_vf_idx = mi300_gpuiov_get_active_vf_idx,
	.get_active_vf_status = mi300_gpuiov_get_active_vf_status,
	.get_time_quanta_index = mi300_gpuiov_get_time_quanta_index,
	.set_time_quanta_index = mi300_gpuiov_set_time_quanta_index,
	.get_time_quanta_definition = mi300_gpuiov_get_time_quanta_definition,
	.set_time_quanta_definition = mi300_gpuiov_set_time_quanta_definition,
	.get_time_quanta_option = mi300_gpuiov_get_time_quanta_option,
	.set_time_quanta_option = mi300_gpuiov_set_time_quanta_option,
	.set_vf_access = mi300_gpuiov_set_vf_access,
	.get_vf_access = mi300_gpuiov_get_vf_access,
	.get_config_info = mi300_gpuiov_get_config_info,
	.wait_auto_sched_stop = mi300_gpuiov_wait_auto_sched_stop,
	.toggle_rlcg_vf_interface = mi300_gpuiov_toggle_rlcg_vf_interface,
	.skip_ctrl_block = mi300_gpuiov_skip_ctrl_block,
};

static int mi300_gpuiov_sw_init(struct amdgv_adapter *adapt)
{
	uint64_t csa_mem_size;
	uint64_t csa_mem_align;

	adapt->gpuiov.funcs = &mi300_gpuiov_funcs;
	//MI300 allocates 256k * num_xcc * 2 (1 vf and pf per xcc)
	csa_mem_size = CSA_SIZE_PER_VF * UNIT_256KB *
			((uint64_t)adapt->mca.max_aid_xcd_num != 0 ? (uint64_t)adapt->mca.max_aid_xcd_num : MI300_MAX_XCD_NUM) * 2;

	csa_mem_align = CSA_SIZE_PER_VF * UNIT_256KB;

	adapt->gpuiov.csa_fb_mem =
		amdgv_memmgr_alloc_align(&adapt->memmgr_pf, csa_mem_size, csa_mem_align,
					MEM_GPUIOV_CSA);
	if (!adapt->gpuiov.csa_fb_mem) {
		AMDGV_ERROR("Failed to reserve memory for CSA\n");
		return AMDGV_FAILURE;
	}

	/* enable config space FLR for KVM only */
	/* on MI300 series, PMFW is integrated in IFWI, so driver needs to first check
	 * if the current PMFW supports config space FLR sequence.
	 */
	if (!(adapt->flags & AMDGV_FLAG_USE_PF)) {
		adapt->flags |= AMDGV_FLAG_FB_CLEAN_ON_SHUTDOWN;
	}

	/* only mi308 support diagnosis data, skip for other series */
	if (adapt->asic_type != CHIP_MI308X) {
		adapt->flags |= AMDGV_FLAG_SKIP_DIAG_DATA;
	}

	return amdgv_gpuiov_ctrl_block_setup(adapt, mi300_hw_sched_static_config, ARRAY_SIZE(mi300_hw_sched_static_config));
}

static int mi300_gpuiov_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->gpuiov.csa_fb_mem) {
		amdgv_memmgr_free(adapt->gpuiov.csa_fb_mem);
		adapt->gpuiov.csa_fb_mem = NULL;
	}
	return 0;
}

static int mi300_gpuiov_hw_fini(struct amdgv_adapter *adapt)
{
	uint32_t strap4;

	if (adapt->flags & AMDGV_FLAG_ENABLE_CFG_FLR_NOTIFY) {
		strap4 = RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP4));
		strap4 &= ~STRAP_FLR_EN_DEV0_F0_MASK;
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP4), strap4);
	}

	if (oss_atomic_read(adapt->in_sync_flood)) {
		AMDGV_INFO("in_sync_flood.gpuiov_hw_fini toggle_vf_mse to false and exit\n");
		if (adapt->vf_rebar_en)
			mi300_gpuiov_toggle_vf_mse(adapt, false);
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

static int mi300_gpuiov_hw_init(struct amdgv_adapter *adapt)
{
	int ret;
	uint32_t xgmi_enable;
	int i;
	uint32_t strap4;
	uint32_t cap;
	uint16_t tmp;

	if (oss_atomic_read(adapt->in_ecc_recovery)) {
		AMDGV_INFO("in_ecc_recovery. mi300_gpuiov_hw_fini first followed by init\n");
		mi300_gpuiov_hw_fini(adapt);
		oss_msleep(500);
	}

	adapt->gpuiov.pos = mi300_gpuiov_find_cap(adapt);
	if (adapt->gpuiov.pos == 0)
		return AMDGV_FAILURE;

	if (amdgv_gpuiov_init(adapt) < 0)
		return AMDGV_FAILURE;

	if (mi300_gpuiov_get_fb_info(adapt))
		return AMDGV_FAILURE;

	/* set the csa's offset and size */
	mi300_gpuiov_set_csa(adapt, adapt->gpuiov.resv_addr);

	// /* enable msix vector 2 */
	// offset = adapt->gpuiov.pos + PCI_GPUIOV_INTR_ENABLE;
	// oss_pci_write_config_dword(adapt->dev, offset, 0x30fff0f);

	/* enable config space FLR for KVM only */
	if (adapt->flags & AMDGV_FLAG_ENABLE_CFG_FLR_NOTIFY) {
		strap4 = RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP4));
		strap4 |= STRAP_FLR_EN_DEV0_F0_MASK;
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP4), strap4);
	}

	mi300_gpuiov_get_sch_offset(adapt);

	if (adapt->asic_type == CHIP_MI300X || adapt->asic_type == CHIP_MI308X) {
		if (adapt->xgmi.phy_nodes_num > 1) {
			if (adapt->psp.program_register) {
				xgmi_enable = 0x1 | (1 << 31);
				adapt->psp.program_register(adapt, AMDGV_PF_IDX, xgmi_enable,
					0, GC_MC_VM_XGMI_GPUIOV_ENABLE);
				adapt->psp.program_register(adapt, AMDGV_PF_IDX, xgmi_enable,
					0, MM_MC_VM_XGMI_GPUIOV_ENABLE);
			}
		}
	}

	/* enable sriov */
	if (!in_whole_gpu_reset()) {
		ret = oss_pci_enable_sriov(adapt->dev, adapt->num_vf);
		if (ret < 0) {
			AMDGV_ERROR("failed to enable sriov with vf num = %d, "
					"ret = %d.\n", adapt->num_vf, ret);
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

	AMDGV_INFO("AID%d: L1 security is %s\n", 0, RREG32(SOC15_REG_OFFSET(MP0, 0, regMP0_SMN_C2PMSG_91)) & 0x4000 ? "ENABLED" : "DISABLED");
	for (i = 1; i < adapt->mcp.num_aid; i++) {
		AMDGV_DEBUG("AID%d: L1 security is %s\n", i, RREG32(SOC15_REG_OFFSET(MP0, i, regMP0_SMN_C2PMSG_91)) & 0x4000 ? "ENABLED" : "DISABLED");
	}

	return 0;
}

struct amdgv_init_func mi300_gpuiov_func = {
	.name = "mi300_gpuiov_func",
	.sw_init = mi300_gpuiov_sw_init,
	.sw_fini = mi300_gpuiov_sw_fini,
	.hw_init = mi300_gpuiov_hw_init,
	.hw_fini = mi300_gpuiov_hw_fini,
};
