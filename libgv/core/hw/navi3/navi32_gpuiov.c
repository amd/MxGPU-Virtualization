/*
 * Copyright (C) 2021-2021  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <amdgv_device.h>
#include <amdgv_gpuiov.h>
#include <amdgv_sched_internal.h>
#include <amdgv.h>
#include "navi32_reg_inc.h"
#include "navi32_gpuiov.h"
#include "navi32_gc.h"
#include "amdgv_mcp.h"

static const int this_block = AMDGV_COMMUNICATION_BLOCK;

/*
 * Redfine auto sched registers to give them better meanings
 * This is due to a limitation in nBIF on Navi32 where
 * we only have 2 blocks of space for auto sched,
 * whcih are used by VCN0 and VCN1
 * MJPEG block has to be programmed by registers
 */
enum {
	regJPEG_GPUIOV_CMD_CONTROL        = regMMSCH_GPUIOV_CMD_CONTROL_2,
	regJPEG_GPUIOV_CMD_STATUS         = regMMSCH_GPUIOV_CMD_STATUS_2,
	regJPEG_GPUIOV_ACTIVE_FUNCTIONS   = regMMSCH_GPUIOV_ACTIVE_FCNS_2,
	regJPEG_GPUIOV_ACTIVE_FUNCTION_ID = regMMSCH_GPUIOV_ACTIVE_FCN_ID_2,
	regJPEG_TIME_QUANTA_OPTION        = regMMSCH_GPUIOV_DW6_2,
	regJPEG_TIME_QUANTA_INDEX         = regMMSCH_GPUIOV_DW7_2,

	regJPEG_GPUIOV_CMD_CONTROL_BASE_IDX        = regMMSCH_GPUIOV_CMD_CONTROL_2_BASE_IDX,
	regJPEG_GPUIOV_CMD_STATUS_BASE_IDX         = regMMSCH_GPUIOV_CMD_STATUS_2_BASE_IDX,
	regJPEG_GPUIOV_ACTIVE_FUNCTIONS_BASE_IDX   = regMMSCH_GPUIOV_ACTIVE_FCNS_2_BASE_IDX,
	regJPEG_GPUIOV_ACTIVE_FUNCTION_ID_BASE_IDX = regMMSCH_GPUIOV_ACTIVE_FCN_ID_2_BASE_IDX,
	regJPEG_TIME_QUANTA_OPTION_BASE_IDX        = regMMSCH_GPUIOV_DW6_2_BASE_IDX,
	regJPEG_TIME_QUANTA_INDEX_BASE_IDX         = regMMSCH_GPUIOV_DW7_2_BASE_IDX,
};

/* CSA size is 256KB per VM */
#define NAVI32_CSA_SIZE_PER_VF	     (256 * 1024)
#define NAVI32_PCI_CONTEXT_LOC_IN_FB  0
#define NAVI32_PCI_CONTEXT_LOC_IN_SYS 1
#define NAVI32_CSA_MAX_VF_NUM         16
/* CSA cntx_size is in units of NAVI32_CSA_SIZE_PER_VF */
#define NAVI32_PCI_CONTEXT_SIZE 1
/* NOTE: cntx_size need to be readback from VBIOS
 *       need PCI_CONTEXT_SIZE[6:0]
 *       however, for navi1x, hw doesn't multicast size the way we want,
 *       so we have to hardcode it.
 *     - RLC & MMSCH ucode hardcode these RO fields
 *       o PCI_CONTEXT_LOC[7]    // is always 0
 *       o PCI_CONTEXT_SIZE[6:0] // is set to agreed per-SOC per-VF CSA area
 *     - GIM/LIBGV will program PCI_CONFIG_SPACE
 *       (for PCI_CONTEXT_OFFSET, PCI_CONTEXT_LOC, PCI_CONTEXT_SIZE)
 *       o Value will multi-cast to MMSCH_GPUIOV_CNTXT,
 *                                  RLC_RLCS_IOV_CNTX_LOC_SIZE,
 *                                  RLC_GPU_IOV_CFG_REG6
 *       o RLC and MMSCH should ignore LOC & SIZE fields
 *         (and use ucode values instead).
 */

#define NAVI32_WAIT_SMU_IDLE_MAX_RETRY 5
#define ENABLE_RLCV_TSL  0x10119
#define DISABLE_RLCV_TSL 0x119

#define NAVI32_AUTO_SCHED_DEBUG_DUMP_MAX_SIZE	40	/* 256MB - 208MB(TMR) - Reserve */
#define NAVI32_AUTO_SCHED_PERF_LOG_SIZE		(AMDGV_MAX_VF_SLOT * 20)	/* 5 dwords for each VF */

#define NAVI32_SUPPORTED_GFX_SCHED_MODE ((1 << AMDGV_SCHED_SOLID_MODE) | (1 << AMDGV_SCHED_LIQUID_MODE) | (1 << AMDGV_SCHED_FAIRNESS) | (1 << AMDGV_SCHED_ROUND_ROBIN))

static struct amdgv_gpuiov_hw_sched_static_config navi32_hw_sched_static_config[NAVI32_HW_SCHED_BLOCK_NUM] = {

	//NAVI32_HW_SCHED_BLOCK_VCN_SCH0_MMSCH
	{"VCN_SCH0_MMSCH",	AMDGV_HW_SCHED_TYPE_MM, AMDGV_SCHED_BLOCK_VCN, AMDGV_SCHED_FRAME_LOOP_MODE, PCI_GPUIOV_UVD0SCH_OFFSET, 0,
				1 << AMDGV_SCHED_FRAME_LOOP_MODE},

	//NAVI32_HW_SCHED_BLOCK_GFX_SCH0_RLCV
	{"GFX_SCH0_RLCV",   AMDGV_HW_SCHED_TYPE_GFX, AMDGV_SCHED_BLOCK_GFX, AMDGV_SCHED_SOLID_MODE, PCI_GPUIOV_GFXSCH_OFFSET, 0,
				NAVI32_SUPPORTED_GFX_SCHED_MODE},

	//NAVI32_HW_SCHED_BLOCK_VCN1_SCH1_MMSCH
	{"VCN1_SCH1_MMSCH", AMDGV_HW_SCHED_TYPE_MM, AMDGV_SCHED_BLOCK_VCN1, AMDGV_SCHED_FRAME_LOOP_MODE, PCI_GPUIOV_UVD1SCH_OFFSET, 0,
				1 << AMDGV_SCHED_FRAME_LOOP_MODE},

	//NAVI32_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH
	{"JPEG_SCH0_MMSCH", AMDGV_HW_SCHED_TYPE_MM,  AMDGV_SCHED_BLOCK_JPEG, AMDGV_SCHED_FRAME_LOOP_MODE, 0, 0,
				1 << AMDGV_SCHED_FRAME_LOOP_MODE},
};

struct navi32_cmd_id navi32_cmd_array[] = {
	{AMDGV_IDLE_GPU,                            NAVI32_IDLE_GPU_LX7,				"IDLE"},
	{AMDGV_SAVE_GPU_STATE,                      NAVI32_SAVE_GPU_STATE_LX7,			"SAVE"},
	{AMDGV_LOAD_GPU_STATE,                      NAVI32_LOAD_GPU_STATE_LX7,			"LOAD"},
	{AMDGV_RUN_GPU,                             NAVI32_RUN_GPU_LX7,					"RUN"},
	{AMDGV_CONTEXT_SWITCH,                      NAVI32_CONTEXT_SWITCH_LX7,			"CONTEXT SWITCH"},
	{AMDGV_ENABLE_AUTO_HW_SWITCH,               NAVI32_ENABLE_AUTO_SCHEDULING,		"ENABLE HW_AUTO_SCHED"},
	{AMDGV_INIT_GPU,                            NAVI32_INIT_GPU_LX7,				"INIT"},
	{AMDGV_DISABLE_AUTO_HW_SCHED,               NAVI32_DISABLE_HW_AUTO_SCHEDULING,	"DISABLE HW_AUTO_SCHED"},
	{AMDGV_SHUTDOWN_GPU,                        NAVI32_SHUTDOWN_GPU_LX7,			"SHUTDOWN VF"},
	{AMDGV_CONFIG_AUTO_HW_SCHED_MODE,           NAVI32_CONFIG_SCHEDULER_FEATURE,	"CONFIG HW_AUTO_SCHED_MODE"},
	{AMDGV_EVENT_NOTIFICATION,                  NAVI32_EVENT_NOTIFICATION,			"EVENT NOTIFICATION"},
	{AMDGV_TRANSFER_VF_DATA,                    NAVI32_TRANSFER_VF_DATA,			"TRANSFER VF DATA"},
};

static int navi32_gpuiov_find_cap(struct amdgv_adapter *adapt)
{
	int pos = 0, found = 0;
	uint32_t vsec_id;

	/* search GPUIOV capability */
	while ((pos = oss_pci_find_next_ext_cap(adapt->dev, pos,
						PCIE_EXT_CAP_ID__VENDOR_SPECIFIC)) != 0) {
		uint32_t vsec = 0;

		oss_pci_read_config_dword(adapt->dev, pos + PCI_GPUIOV_VSEC, &vsec);
		vsec_id = PCI_GPUIOV_VSEC__ID(vsec);

		if (vsec_id == PCI_GPUIOV_VSEC__ID__GPU_IOV) {
			found = 1;
			break;
		}
	}

	if (!found) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_IOV_NO_GPU_IOV_CAP, 0);
		return 0;
	}

	return pos;
}

static int navi32_gpuiov_get_sched_block_offset(struct amdgv_adapter *adapt,
					       uint32_t hw_sched_id)
{
	if (hw_sched_id >= adapt->gpuiov.num_ctrl_blocks) {
		AMDGV_ERROR("%s(%d) is an invalid scheduler for this operation\n",
			amdgv_hw_sched_id_to_name(adapt, hw_sched_id), hw_sched_id);
		return AMDGV_FAILURE;
	}

	return (adapt->gpuiov.pos + \
		(adapt->gpuiov.ctrl_blocks[hw_sched_id].offset << 4));
}

static enum navi32_gpuiov_cmd_lx7 navi32_decode_cmd_lx7(enum amdgv_gpuiov_cmd cmd)
{
	uint32_t i;
	uint32_t size = ARRAY_SIZE(navi32_cmd_array);

	for (i = 0; i < size; ++i) {
		if (navi32_cmd_array[i].cmd == cmd)
			return navi32_cmd_array[i].navi32_cmd;
	}

	return NAVI32_INVALID_COMMAND;
}

static const char *navi32_gpuiov_cmd_to_name(struct amdgv_adapter *adapt, uint32_t cmd, uint32_t hw_sched_id)
{
	if (hw_sched_id == NAVI32_HW_SCHED_BLOCK_GFX_SCH0_RLCV) {
		uint32_t i;
		uint32_t size = ARRAY_SIZE(navi32_cmd_array);

		for (i = 0; i < size; ++i) {
			if (navi32_cmd_array[i].cmd == cmd)
				return navi32_cmd_array[i].name;
		}
		return "INVALID CMD";
	}

	return amdgv_gpuiov_cmd_to_name_default(adapt, cmd, hw_sched_id);
}

static int navi32_gpuiov_write_cmd_data(struct amdgv_adapter *adapt, uint32_t hw_sched_id,
				uint32_t idx_vf, uint32_t data)
{
	uint32_t cmd_ctrl_offset, cmd_status_offset;
	uint32_t offset;

	if (adapt->rlcv_stamp_todo != adapt->rlcv_stamp_status) {
		int temp_data;
		offset = navi32_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
		cmd_ctrl_offset = PCI_SCH_CMD_CONTROL + offset;
		if (adapt->rlcv_stamp_todo)
			temp_data = ENABLE_RLCV_TSL;
		else
			temp_data = DISABLE_RLCV_TSL;

		oss_pci_write_config_dword(adapt->dev, cmd_ctrl_offset, temp_data);
		AMDGV_WARN("idx_vf=%d hw_sched_id=%d (%s) pci_write_config_dword(0x%08x, 0x%08x)\n",
			idx_vf, hw_sched_id, amdgv_hw_sched_id_to_name(adapt, hw_sched_id),
			cmd_ctrl_offset, temp_data);
		oss_udelay(20);
		adapt->rlcv_stamp_status = adapt->rlcv_stamp_todo;
	}

	if (hw_sched_id == NAVI32_HW_SCHED_BLOCK_GFX_SCH0_RLCV) {
		/* GFX use MMIO-IOV */
		AMDGV_DEBUG("MMIO write(0x%08x)\n", data);
			WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPU_IOV_CFG_REG1), data);
	} else if (hw_sched_id == NAVI32_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) {
		/* JPEG uses GPUIOV registers */
		AMDGV_DEBUG("JPEG GPUIOV reg write(0x%08x)\n", data);
		WREG32(SOC15_REG_OFFSET(VCN, 0, regJPEG_GPUIOV_CMD_CONTROL), data);
	} else {	/* for multimedia, still use GPU-IOV */
		offset = navi32_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
		if (offset == AMDGV_FAILURE) {
			AMDGV_ERROR("wrong offset for hw_sched_id=%d\n", hw_sched_id);
			return AMDGV_FAILURE;
		}

		/* clear GPUIOV CMD STATUS */
		cmd_status_offset = PCI_SCH_CMD_STATUS + offset;
		oss_pci_write_config_byte(adapt->dev, cmd_status_offset, AMDGV_CMD_STATUS_PENDING_EXECUTE);
		cmd_ctrl_offset = PCI_SCH_CMD_CONTROL + offset;
		AMDGV_DEBUG("PCIE write(0x%08x, 0x%08x)\n", cmd_ctrl_offset, data);
		oss_pci_write_config_dword(adapt->dev, cmd_ctrl_offset, data);
		AMDGV_DEBUG("PCIE write(0x%08x, 0x%08x)\n", cmd_ctrl_offset, data);

		/* Add to diagnosis data */
		AMDGV_DIAG_DATA_TRACE_LOG_GPUIOV_CMD_START(idx_vf, hw_sched_id, cmd, data);
	}

	return 0;
}

static int navi32_gpuiov_set_cmd(struct amdgv_adapter *adapt, enum amdgv_gpuiov_cmd cmd,
				uint32_t hw_sched_id, uint32_t idx_vf,
				uint32_t next_idx_vf)
{
	uint32_t data, func_id, next_func_id;
	enum navi32_gpuiov_cmd_lx7 cmd_lx7;

	/* write CMD */
	func_id = PCI_GPUIOV_FUNC_ID(idx_vf);
	next_func_id = PCI_GPUIOV_FUNC_ID(next_idx_vf);

	/* only rlcv uses lx7 decoding */
	if (IS_HW_SCHED_TYPE_GFX(hw_sched_id)) {
		cmd_lx7 = navi32_decode_cmd_lx7(cmd);
		if (cmd_lx7 == NAVI32_INVALID_COMMAND)
			goto out;

		/* Use idx of vf directly, otherwise decoding needed in RLCV */
		if (cmd_lx7 == NAVI32_CONFIG_SCHEDULER_FEATURE) {
			func_id = idx_vf;
			next_func_id = next_idx_vf;
		}
	}

	data = (next_func_id << 16) | (func_id << 8) | CMD_EXECUTE | (cmd & 0x0F);

	AMDGV_DEBUG("idx_vf=%d hw_sched_id=%d (%s) cmd=0x%x (%s) func_id=0x%x "
				"next_func_id=0x%x\n",
				idx_vf, hw_sched_id, amdgv_hw_sched_id_to_name(adapt, hw_sched_id), cmd,
				amdgv_gpuiov_cmd_to_name(adapt, cmd, hw_sched_id), func_id, next_func_id);

	/* Add to diagnosis data */
	AMDGV_DIAG_DATA_TRACE_LOG_GPUIOV_CMD_START(idx_vf, hw_sched_id, cmd, data);

	if (navi32_gpuiov_write_cmd_data(adapt, hw_sched_id, idx_vf, data))
		return AMDGV_FAILURE;

out:
	adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd = cmd;
	adapt->gpuiov.ctrl_blocks[hw_sched_id].last_status = AMDGV_CMD_STATUS_PENDING_EXECUTE;

	return 0;
}

static int navi32_set_event_notification(struct amdgv_adapter *adapt,
				uint32_t hw_sched_id, uint32_t idx_vf, enum amdgv_gpuiov_event_id event_id,
				uint32_t value)
{
	uint32_t cmd = AMDGV_EVENT_NOTIFICATION;
	uint32_t data;

	AMDGV_DEBUG("event notification event_id=%d hw_sched_id=%d (%s) "
		    "cmd=0x%x (%s), NEXT_FCN_ID(val)=0x%x",
		    event_id, hw_sched_id, amdgv_hw_sched_id_to_name(adapt, hw_sched_id),
		    cmd, amdgv_gpuiov_cmd_to_name(adapt, cmd, hw_sched_id), value);

	/* write GPUIOV_CMD */
	data = (value << 16) | (event_id << 8) | CMD_EXECUTE | (cmd & 0x0F);

	if (navi32_gpuiov_write_cmd_data(adapt, hw_sched_id, idx_vf, data))
		return AMDGV_FAILURE;

	adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd = cmd;
	adapt->gpuiov.ctrl_blocks[hw_sched_id].last_status = AMDGV_CMD_STATUS_PENDING_EXECUTE;

	return 0;
}

static void navi32_dump_rlcv_sram(struct amdgv_adapter *adapt)
{
	int i;
	int ret;

	WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPU_IOV_SCRATCH_ADDR), 0x0);
	for (i = 0; i < CSA_TSL_SIZE / 4; i++) {
		adapt->rlcv_ts_buff[i] = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPU_IOV_SCRATCH_DATA));
	}
	ret = oss_store_rlcv_timestamp((char *)adapt->rlcv_ts_buff, CSA_TSL_SIZE, adapt->bdf);
	if (ret)
		AMDGV_WARN("store rlcv timestamp failed, ret=%d\n", ret);
}

static int navi32_gpuiov_setup_sched_debug_log(struct amdgv_adapter *adapt,
				enum amdgv_auto_sched_log_op op)
{
	int ret = 0;
	uint32_t world_switch_id;
	struct amdgv_sched_world_switch *world_switch;

	switch (op) {
	case AMDGV_AUTO_SCHED_PERF_LOG:
		if (adapt->gpuiov.perf_log_mem)
			return 0;

		adapt->gpuiov.perf_log_mem =
			amdgv_memmgr_alloc_align(&adapt->memmgr_pf, NAVI32_AUTO_SCHED_PERF_LOG_SIZE,
						PAGE_SIZE, MEM_GPUIOV_SCHED_LOG);
		if (!adapt->gpuiov.perf_log_mem) {
			AMDGV_ERROR("Failed to allocate debug dump memory!\n");
			return AMDGV_FAILURE;
		}
		AMDGV_DEBUG("PERF LOG MEM: GPU_ADDR=0x%llx MEM_ADDR=0x%llx MEM_SIZE=0x%llx\n",
				amdgv_memmgr_get_gpu_addr(adapt->gpuiov.perf_log_mem),
				amdgv_memmgr_get_offset(adapt->gpuiov.perf_log_mem),
				amdgv_memmgr_get_size(adapt->gpuiov.perf_log_mem));
		break;

	case AMDGV_AUTO_SCHED_DEBUG_DUMP:
		if (adapt->gpuiov.debug_dump_mem)
			return 0;

		if (adapt->opt.debug_dump_reserve_size > NAVI32_AUTO_SCHED_DEBUG_DUMP_MAX_SIZE) {
			AMDGV_WARN("For Navi32 the max usable size is 40MB. Setting debug_dump_reserve_size to 40MB\n");
			adapt->opt.debug_dump_reserve_size = NAVI32_AUTO_SCHED_DEBUG_DUMP_MAX_SIZE;
		}

		adapt->gpuiov.debug_dump_mem =
			amdgv_memmgr_alloc_align(&adapt->memmgr_pf, MBYTES_TO_BYTES(adapt->opt.debug_dump_reserve_size),
						1 << 20, MEM_GPUIOV_SCHED_LOG);
		if (!adapt->gpuiov.debug_dump_mem) {
			AMDGV_ERROR("Failed to allocate debug dump memory!\n");
			return AMDGV_FAILURE;
		}
		AMDGV_DEBUG("DEBUG DUMP MEM: GPU_ADDR=0x%llx MEM_ADDR=0x%llx MEM_SIZE=0x%llx\n",
				amdgv_memmgr_get_gpu_addr(adapt->gpuiov.debug_dump_mem),
				amdgv_memmgr_get_offset(adapt->gpuiov.debug_dump_mem),
				amdgv_memmgr_get_size(adapt->gpuiov.debug_dump_mem));
		break;

	default:
		AMDGV_ERROR("Invalid debug log type %d\n", op);
		return AMDGV_FAILURE;
	}

	for_each_id(world_switch_id, amdgv_sched_get_world_switch_mask_by_sched_block(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX)) {
		world_switch = &adapt->sched.world_switch[world_switch_id];
		if (world_switch->sched_mode > AMDGV_SCHED_MAX_HW_SCHED_MODE)
			continue;
		ret = amdgv_sched_world_switch_config_auto_sched_mode(adapt, world_switch);
		if (ret)
			return ret;
	}

	return 0;
}

static bool navi32_gpuiov_is_cmd_complete(struct amdgv_adapter *adapt,
					 uint32_t hw_sched_id)
{
	uint8_t command, status;
	uint32_t resp;
	int cmd_ctrl_offset, cmd_status_offset;
	int offset;
	enum navi32_gpuiov_cmd_lx7 cmd_lx7;
	uint32_t last_status_reg = 0;
	if (hw_sched_id == NAVI32_HW_SCHED_BLOCK_GFX_SCH0_RLCV) {
		/*  If we read status before response and then check the execution bit,
		 *  this will leave a few microsec gap
		 *  within which the status bit might still change
		 */
		resp = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPU_IOV_CFG_REG1));
		last_status_reg = RREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPU_IOV_CFG_REG2));
	} else if  (hw_sched_id == NAVI32_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) {
		resp = RREG32(SOC15_REG_OFFSET(VCN, 0, regJPEG_GPUIOV_CMD_CONTROL));
		last_status_reg = RREG32(SOC15_REG_OFFSET(VCN, 0, regJPEG_GPUIOV_CMD_STATUS));
	} else {
		offset = navi32_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
		if (offset == AMDGV_FAILURE) {
			AMDGV_ERROR("wrong offset for hw_sched_id=%d\n", hw_sched_id);
			return false;
		}

		cmd_ctrl_offset = PCI_SCH_CMD_CONTROL + offset;
		oss_pci_read_config_dword(adapt->dev, cmd_ctrl_offset, &resp);

		cmd_status_offset = PCI_SCH_CMD_STATUS + offset;
		oss_pci_read_config_byte(adapt->dev, cmd_status_offset, &status);
		last_status_reg = status;
	}
	adapt->gpuiov.ctrl_blocks[hw_sched_id].last_status_reg = last_status_reg;
	status = adapt->gpuiov.ctrl_blocks[hw_sched_id].last_status;
	if (resp & CMD_EXECUTE) /* NOT STARTED processing CMD */
		return false;

	command = adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd;
	cmd_lx7 = navi32_decode_cmd_lx7(command);
	if (cmd_lx7 == NAVI32_INVALID_COMMAND)
		return true;
	if ((status == 0) && adapt->rlcv_stamp_status) {
		if (cmd_lx7 == NAVI32_RUN_GPU_LX7) {
			adapt->rlcv_stamp_count++;
			if (adapt->rlcv_stamp_count > 0) {
				navi32_dump_rlcv_sram(adapt);
			}
		}
	}

	if (status == 0)
		return true;
	else
		return false;
}

void navi32_dump_gpuiov_cmd_status(struct amdgv_adapter *adapt, uint32_t hw_sched_id)
{
	uint8_t command, status;
	uint32_t last_status_reg = adapt->gpuiov.ctrl_blocks[hw_sched_id].last_status_reg;
	command = adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd;
	status = adapt->gpuiov.ctrl_blocks[hw_sched_id].last_status;

	AMDGV_INFO("hw_sched_id=%d (%s) GPUIOV_CMD=%d (%s) CMD_STATUS=%d (%s) last_status_reg=0x%08x\n", hw_sched_id,
		    amdgv_hw_sched_id_to_name(adapt, hw_sched_id), command,
		    amdgv_gpuiov_cmd_to_name(adapt, command, hw_sched_id), status,
		    amdgv_gpuiov_status_to_name(status), last_status_reg);
}

static int navi32_gpuiov_set_total_fb_consumed(struct amdgv_adapter *adapt,
					      uint16_t total_fb_consumed)
{
	uint32_t offset = adapt->gpuiov.pos + PCI_GPUIOV_TOTAL_FB_CONSUMED;

	return oss_pci_write_config_word(adapt->dev, offset, total_fb_consumed);
}

static int navi32_gpuiov_get_total_fb_consumed(struct amdgv_adapter *adapt,
					      uint16_t *total_fb_consumed)
{
	uint32_t offset = adapt->gpuiov.pos + PCI_GPUIOV_TOTAL_FB_CONSUMED;

	return oss_pci_read_config_word(adapt->dev, offset, total_fb_consumed);
}

static int navi32_gpuiov_set_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf,
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

static int navi32_gpuiov_get_vf_fb(struct amdgv_adapter *adapt, uint32_t idx_vf,
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

static int navi32_gpuiov_get_vm_busy_status(struct amdgv_adapter *adapt,
					   uint32_t hw_sched_id,
					   uint32_t *vm_busy_status)
{
	int offset = navi32_gpuiov_get_sched_block_offset(adapt, hw_sched_id);

	if (offset == AMDGV_FAILURE) {
		AMDGV_ERROR("wrong offset for hw_sched_id=%d\n", hw_sched_id);
		return AMDGV_FAILURE;
	}

	offset += PCI_SCH_VM_BUSY_STATUS;

	return oss_pci_read_config_dword(adapt->dev, offset, vm_busy_status);
}

static int navi32_gpuiov_clear_intr_status(struct amdgv_adapter *adapt, uint32_t bits)
{
	uint32_t offset;
	uint32_t hvvm_mask = (1 << 24) | (1 << 25);

	/* Set GFX INTR_ENABLE */
	offset = adapt->gpuiov.pos + PCI_GPUIOV_ENG_A_INTR_STATUS;
	oss_pci_write_config_dword(adapt->dev, offset, bits & 0xF);

	/* Set VCN0/1 INTR_ENABLE */
	offset = adapt->gpuiov.pos + PCI_GPUIOV_ENG_B_INTR_STATUS;
	oss_pci_write_config_dword(adapt->dev, offset, (bits >> 8) & 0xFF);

	/* Set HVVM MAILBOX bits */
	offset = adapt->gpuiov.pos + PCI_GPUIOV_INTR_STATUS;
	oss_pci_write_config_dword(adapt->dev, offset, bits & hvvm_mask);

	return 0;
}

static int navi32_gpuiov_get_intr_bits(struct amdgv_adapter *adapt, uint32_t *bits)
{
	uint32_t offset;
	uint32_t temp_bits = 0;

	/* Get GFX INTR_ENABLE */
	offset = adapt->gpuiov.pos + PCI_GPUIOV_ENG_A_INTR_ENABLE;
	oss_pci_read_config_dword(adapt->dev, offset, &temp_bits);
	*bits = temp_bits;

	/* Get VCN0/1 INTR_ENABLE */
	offset = adapt->gpuiov.pos + PCI_GPUIOV_ENG_B_INTR_ENABLE;
	oss_pci_read_config_dword(adapt->dev, offset, &temp_bits);
	*bits |= (temp_bits << 8);

	/* Get HVVM MAILBOX bits */
	offset = adapt->gpuiov.pos + PCI_GPUIOV_INTR_ENABLE;
	oss_pci_read_config_dword(adapt->dev, offset, &temp_bits);
	*bits |= temp_bits;

	return 0;
}

static int navi32_gpuiov_set_intr_bits(struct amdgv_adapter *adapt, uint32_t bits)
{
	uint32_t offset;
	uint32_t hvvm_mask = (1 << 24) | (1 << 25);

	/* Set GFX INTR_ENABLE */
	offset = adapt->gpuiov.pos + PCI_GPUIOV_ENG_A_INTR_ENABLE;
	oss_pci_write_config_dword(adapt->dev, offset, bits & 0xF);

	/* Set VCN0/1 INTR_ENABLE */
	offset = adapt->gpuiov.pos + PCI_GPUIOV_ENG_B_INTR_ENABLE;
	oss_pci_write_config_dword(adapt->dev, offset, (bits >> 8) & 0xFF);

	/* Set HVVM MAILBOX bits */
	offset = adapt->gpuiov.pos + PCI_GPUIOV_INTR_ENABLE;
	oss_pci_write_config_dword(adapt->dev, offset, bits & hvvm_mask);

	return 0;
}

static int navi32_gpuiov_get_intr_status(struct amdgv_adapter *adapt, uint32_t *bits)
{
	uint32_t offset;
	uint32_t temp_bits = 0;

	/* Get GFX INTR_ENABLE */
	offset = adapt->gpuiov.pos + PCI_GPUIOV_ENG_A_INTR_STATUS;
	oss_pci_read_config_dword(adapt->dev, offset, &temp_bits);
	*bits = temp_bits;

	/* Get VCN0/1 INTR_ENABLE */
	offset = adapt->gpuiov.pos + PCI_GPUIOV_ENG_B_INTR_STATUS;
	oss_pci_read_config_dword(adapt->dev, offset, &temp_bits);
	*bits |= (temp_bits << 8);

	/* Get HVVM MAILBOX bits */
	offset = adapt->gpuiov.pos + PCI_GPUIOV_INTR_STATUS;
	oss_pci_read_config_dword(adapt->dev, offset, &temp_bits);
	*bits |= temp_bits;

	return 0;
}

static int navi32_gpuiov_get_hvvm_mbox_index(struct amdgv_adapter *adapt, uint8_t *index)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_HVVM_MBOX0;

	return oss_pci_read_config_byte(adapt->dev, offset, index);
}

static int navi32_gpuiov_update_hvvm_mbox_index(struct amdgv_adapter *adapt, uint8_t index)
{
	uint32_t offset;

	offset = adapt->gpuiov.pos + PCI_GPUIOV_HVVM_MBOX0;

	return oss_pci_write_config_byte(adapt->dev, offset, index);
}

int navi32_gpuiov_rcv_hvvm_mbox_msg(struct amdgv_adapter *adapt, uint8_t *msg_data)
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

int navi32_gpuiov_trn_hvvm_mbox_data(struct amdgv_adapter *adapt, uint8_t msg_data)
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

int navi32_gpuiov_set_hvvm_mbox_valid(struct amdgv_adapter *adapt, uint8_t bits)
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

int navi32_gpuiov_set_hvvm_mbox_ack(struct amdgv_adapter *adapt)
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

static int navi32_gpuiov_get_hvvm_mbox_msg_valid(struct amdgv_adapter *adapt, uint32_t *bit_map)
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

static int navi32_gpuiov_get_time_quanta_index(struct amdgv_adapter *adapt, uint32_t idx_vf,
					      uint32_t hw_sched_id,
					      uint32_t *time_quanta_index)
{
	uint32_t data;
	int offset;
	int bshift;

	bshift = PCI_SCH_TIME_QUANTA_INDEX_SHIFT(idx_vf);

	if  (hw_sched_id ==  NAVI32_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) {
		data = RREG32(SOC15_REG_OFFSET(VCN, 0, regJPEG_TIME_QUANTA_INDEX));
	} else {
		offset = navi32_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
		if (offset == AMDGV_FAILURE) {
			AMDGV_ERROR("wrong offset for hw_sched_id=%d\n", hw_sched_id);
			return AMDGV_FAILURE;
		}

		offset += PCI_SCH_TIME_QUANTA_INDEX(idx_vf);

		oss_pci_read_config_dword(adapt->dev, offset, &data);
	}

	/* time_quanta_index is 2-bits (for time_quanta[3:0] */
	*time_quanta_index = (data >> bshift) & 0x3;

	return 0;
}

static int navi32_gpuiov_set_time_quanta_index(struct amdgv_adapter *adapt, uint32_t idx_vf,
					      uint32_t hw_sched_id,
					      uint32_t time_quanta_index)
{
	uint32_t data;
	int offset;
	int bshift;

	bshift = PCI_SCH_TIME_QUANTA_INDEX_SHIFT(idx_vf);

	if  (hw_sched_id ==  NAVI32_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) {
		data = RREG32(SOC15_REG_OFFSET(VCN, 0, regJPEG_TIME_QUANTA_INDEX));
		data &= ~(0x3 << bshift);
		data |= ((time_quanta_index & 0x3) << bshift);
		WREG32(SOC15_REG_OFFSET(VCN, 0, regJPEG_TIME_QUANTA_INDEX), data);
	} else {
		offset = navi32_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
		if (offset == AMDGV_FAILURE) {
			AMDGV_ERROR("wrong offset for hw_sched_id=%d\n", hw_sched_id);
			return AMDGV_FAILURE;
		}

		offset += PCI_SCH_TIME_QUANTA_INDEX(idx_vf);

		oss_pci_read_config_dword(adapt->dev, offset, &data);
		/* time_quanta_index is 2-bits (for time_quanta[3:0] */
		data &= ~(0x3 << bshift);
		data |= ((time_quanta_index & 0x3) << bshift);
		oss_pci_write_config_dword(adapt->dev, offset, data);
	}

	AMDGV_DEBUG("hw_sched_id=%d (%s) %s time_quanta_index=%d\n", hw_sched_id,
		    amdgv_hw_sched_id_to_name(adapt, hw_sched_id), amdgv_idx_to_str(idx_vf),
		    time_quanta_index);
	return 0;
}

static int navi32_gpuiov_get_time_quanta_option(struct amdgv_adapter *adapt,
					       uint32_t hw_sched_id,
					       uint32_t *time_quanta_option)
{
	int offset;

	if  (hw_sched_id ==  NAVI32_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) {
		*time_quanta_option = RREG32(SOC15_REG_OFFSET(VCN, 0, regJPEG_TIME_QUANTA_OPTION));
	} else {
		offset = navi32_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
		if (offset == AMDGV_FAILURE) {
			AMDGV_ERROR("wrong offset for hw_sched_id=%d\n", hw_sched_id);
			return AMDGV_FAILURE;
		}

		offset += PCI_SCH_TIME_QUANTA_OPTION;
		oss_pci_read_config_dword(adapt->dev, offset, time_quanta_option);
	}

	return 0;
}

static int navi32_gpuiov_set_time_quanta_option(struct amdgv_adapter *adapt,
					       uint32_t hw_sched_id,
					       uint32_t time_quanta_option)
{
	int offset;

	if  (hw_sched_id ==  NAVI32_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) {
		WREG32(SOC15_REG_OFFSET(VCN, 0, regJPEG_TIME_QUANTA_OPTION), time_quanta_option);
	} else if (hw_sched_id ==  NAVI32_HW_SCHED_BLOCK_GFX_SCH0_RLCV) {
		offset = navi32_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
		if (offset == AMDGV_FAILURE) {
			AMDGV_ERROR("wrong offset for hw_sched_id=%d\n", hw_sched_id);
			return AMDGV_FAILURE;
		}
		offset += PCI_SCH_TIME_QUANTA_OPTION;
		oss_pci_write_config_dword(adapt->dev, offset, time_quanta_option);
	} else {
		offset = navi32_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
		if (offset == AMDGV_FAILURE) {
			AMDGV_ERROR("wrong offset for hw_sched_id=%d\n", hw_sched_id);
			return AMDGV_FAILURE;
		}

		offset += PCI_SCH_TIME_QUANTA_OPTION;
		oss_pci_write_config_dword(adapt->dev, offset, time_quanta_option);
	}

	AMDGV_DEBUG("hw_sched_id=%d (%s) time_quanta_option=0x%08x)\n", hw_sched_id,
		    amdgv_hw_sched_id_to_name(adapt, hw_sched_id), time_quanta_option);
	return 0;
}

static int navi32_gpuiov_get_active_vfs(struct amdgv_adapter *adapt,
				       uint32_t hw_sched_id, uint32_t *active_vfs)
{
	int offset;

	if  (hw_sched_id ==  NAVI32_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) {
		*active_vfs = RREG32(SOC15_REG_OFFSET(VCN, 0, regJPEG_GPUIOV_ACTIVE_FUNCTIONS));
	} else if (hw_sched_id == NAVI32_HW_SCHED_BLOCK_GFX_SCH0_RLCV) {
		*active_vfs = adapt->gpuiov.sched_cfg.auto_config.active_functions;
	} else {
		offset = navi32_gpuiov_get_sched_block_offset(adapt, hw_sched_id) +
			PCI_SCH_ACTIVE_FUNCTIONS;
		oss_pci_read_config_dword(adapt->dev, offset, active_vfs);
	}

	return 0;
}

static int navi32_gpuiov_set_active_vfs(struct amdgv_adapter *adapt,
				       uint32_t hw_sched_id, uint32_t active_vfs)
{
	int offset;

	if  (hw_sched_id ==  NAVI32_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) {
		WREG32(SOC15_REG_OFFSET(VCN, 0, regJPEG_GPUIOV_ACTIVE_FUNCTIONS), active_vfs);
	} else if (hw_sched_id == NAVI32_HW_SCHED_BLOCK_GFX_SCH0_RLCV) {
		adapt->gpuiov.sched_cfg.auto_config.active_functions = active_vfs;
	} else {
		offset = navi32_gpuiov_get_sched_block_offset(adapt, hw_sched_id) +
			PCI_SCH_ACTIVE_FUNCTIONS;
		oss_pci_write_config_dword(adapt->dev, offset, active_vfs);
	}

	return 0;
}

static int navi32_gpuiov_add_active_vf(struct amdgv_adapter *adapt,
				      uint32_t hw_sched_id, uint32_t idx_vf)
{
	uint32_t active_vfs = 0;

	navi32_gpuiov_get_active_vfs(adapt, hw_sched_id, &active_vfs);

	if (idx_vf == AMDGV_PF_IDX)
		active_vfs |= (1U << 31);
	else
		active_vfs |= (1U << idx_vf);

	navi32_gpuiov_set_active_vfs(adapt, hw_sched_id, active_vfs);
	return 0;
}

static int navi32_gpuiov_remove_active_vf(struct amdgv_adapter *adapt,
					 uint32_t hw_sched_id, uint32_t idx_vf)
{
	uint32_t active_vfs = 0;

	navi32_gpuiov_get_active_vfs(adapt, hw_sched_id, &active_vfs);

	if (idx_vf == AMDGV_PF_IDX)
		active_vfs &= ~(1U << 31);
	else
		active_vfs &= ~(1U << idx_vf);

	navi32_gpuiov_set_active_vfs(adapt, hw_sched_id, active_vfs);
	return 0;
}

static int navi32_gpuiov_get_active_vf_idx(struct amdgv_adapter *adapt,
					  uint32_t hw_sched_id, uint32_t *idx_vf)
{
	int offset;
	uint32_t data;

	if  (hw_sched_id ==  NAVI32_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) {
		data = RREG32(SOC15_REG_OFFSET(VCN, 0, regJPEG_GPUIOV_ACTIVE_FUNCTION_ID));
	} else {
		offset = navi32_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
		if (offset == AMDGV_FAILURE) {
			AMDGV_ERROR("wrong offset for hw_sched_id=%d\n", hw_sched_id);
			return AMDGV_FAILURE;
		}

		offset += PCI_SCH_ACTIVE_FUNCTION_ID;
		oss_pci_read_config_dword(adapt->dev, offset, &data);
	}

	switch (adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_sched_type) {
	case AMDGV_HW_SCHED_TYPE_GFX:
		if (data & 0x80000000)
			*idx_vf = data & 0x1F;
		else {
			/* in case of the timeout happens with pf mode but active one is vf,
			 * Add WS VF ID in bitfield definition as below:
			 *
			 * VF_ID[7:0]                       // supports up to 128 VFs, but max is 127 VFs.
			 * ACTIVE_FCN_ID_STATUS [11:8]
			 * Reserved [15:12]
			 * WS_VF_ID[23:16]                  // supports up to 128 VFs, but max is 127 VFs.
			 * Reserved [29:24]
			 * WS_PF_VF[30]
			 * PF_VF = [31]
			 */
			if (data & 0x40000000)
				*idx_vf = (data & 0xFF0000) >> 16;
			else
				*idx_vf = AMDGV_PF_IDX;
		}
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

	AMDGV_DEBUG("PCI_SCH_ACTIVE_FUNCTION_ID: readback=0x%08x, idx_vf=%d\n",
			data, *idx_vf);

	return 0;
}

static int navi32_gpuiov_get_active_vf_status(struct amdgv_adapter *adapt,
					     uint32_t hw_sched_id, uint8_t *status)
{
	int offset;

	if  (hw_sched_id ==  NAVI32_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) {
		*status = RREG32(SOC15_REG_OFFSET(VCN, 0, regJPEG_GPUIOV_ACTIVE_FUNCTION_ID)) & REG_GPUIOV_VF_STATUS_MASK;
	} else {
		offset = navi32_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
		if (offset == AMDGV_FAILURE) {
			AMDGV_ERROR("wrong offset for hw_sched_id=%d\n", hw_sched_id);
			return AMDGV_FAILURE;
		}

		offset += PCI_SCH_ACTIVE_FUNCTION_ID_STATUS;
		oss_pci_read_config_byte(adapt->dev, offset, status);
	}

	AMDGV_DEBUG("PCI_SCH_ACTIVE_FUNCTION_ID_STATUS: readback=0x%04x\n", *status);

	return 0;
}

static int navi32_gpuiov_set_scheduler_config_descriptor(struct amdgv_adapter *adapt,
							uint32_t hw_sched_id, struct scheduler_memory_descriptor *sched_cfg)
{
	int offset;
	uint64_t fb_addr;

	if (hw_sched_id != NAVI32_HW_SCHED_BLOCK_GFX_SCH0_RLCV)
		return 0;

	fb_addr = amdgv_memmgr_get_gpu_addr(adapt->gpuiov.sched_cfg_mem) - adapt->memmgr_pf.mc_base;
	oss_memset((uint8_t *)adapt->fb + fb_addr, 0, sizeof(struct scheduler_memory_descriptor));
	oss_memcpy((uint8_t *)adapt->fb + fb_addr, sched_cfg, sizeof(struct scheduler_memory_descriptor));

	/* In NV32 auto schedule design, RLC_GPU_IOV_SCH_1 usage is different from original spec.
	 * It is now used to store the base address of the scheduler config descriptor in 4K unit.
	 * Register RLC_GPU_IOV_SCH_1 is mapped to PCI_SCH_TIME_QUANTA_INDEX(0-15) in PCI config space.
	 */
	offset = navi32_gpuiov_get_sched_block_offset(adapt, hw_sched_id) +
			PCI_SCH_TIME_QUANTA_INDEX(0);
	if (offset == AMDGV_FAILURE) {
		AMDGV_ERROR("wrong offset for hw_sched_id=%d\n", hw_sched_id);
		return AMDGV_FAILURE;
	}

	oss_pci_write_config_dword(adapt->dev, offset, fb_addr >> 12);

	WREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_HDP_MEM_COHERENCY_FLUSH_CNTL), 0x0);

	return 0;
}

static int navi32_gpuiov_wait_auto_sched_stop(struct amdgv_adapter *adapt,
					     uint32_t hw_sched_id)
{
	struct amdgv_sched_world_switch *world_switch;
	int offset;
	int wait_ret;
	uint64_t timeout;

	if (amdgv_sched_get_world_switch_by_hw_sched_id(adapt, hw_sched_id, &world_switch))
		return AMDGV_FAILURE;

	if (!world_switch->enabled)
		return AMDGV_FAILURE;

	if (hw_sched_id == NAVI32_HW_SCHED_BLOCK_GFX_SCH0_RLCV)
		timeout = AMDGV_TIMEOUT(TIMEOUT_AUTO_SWITCH_GFX);
	else
		timeout = AMDGV_TIMEOUT(TIMEOUT_AUTO_SWITCH_MM);

	if  (hw_sched_id ==  NAVI32_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) {
		wait_ret = amdgv_wait_for_register(adapt, SOC15_REG_OFFSET(VCN, 0, regJPEG_GPUIOV_ACTIVE_FUNCTION_ID),
										REG_GPUIOV_VF_STATUS_MASK, 0, timeout, AMDGV_WAIT_CHECK_EQ, 0);
	} else {
		offset = navi32_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
		if (offset == AMDGV_FAILURE) {
			AMDGV_ERROR("Cannot find offset for %s scheduler in PCIe config\n",
				amdgv_hw_sched_id_to_name(adapt, hw_sched_id));
			return AMDGV_FAILURE;
		}

		offset += PCI_SCH_ACTIVE_FUNCTION_ID_STATUS;

		wait_ret = amdgv_wait_for_pci_cfg(adapt, adapt->dev, offset, 0xf, 0, 1, timeout, AMDGV_WAIT_CHECK_EQ, 0);
	}

	if (wait_ret)
		AMDGV_WARN("Fail to wait auto sched %s scheduler stop\n",
			amdgv_hw_sched_id_to_name(adapt, hw_sched_id));

	return wait_ret;
}

static int navi32_gpuiov_set_vf_access(struct amdgv_adapter *adapt, uint32_t idx_vf,
				      uint32_t vf_access_select, bool is_true)
{
	uint32_t tmp, reg_addr, value, origin;

	if (idx_vf == AMDGV_PF_IDX)
		return 0;
	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return AMDGV_FAILURE;
	if (adapt->flags & AMDGV_FLAG_DISABLE_MMIO_PROTECTION) {
		AMDGV_DEBUG("force disable vf mmio protection\n");
		vf_access_select = AMDGV_VF_ACCESS_ALL;
		is_true = true;
	}

	value = (uint32_t)0x1 << idx_vf;

	if (vf_access_select & AMDGV_VF_ACCESS_FB) {
		reg_addr = SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_VF_FB_EN);
		tmp = RREG32(reg_addr);
		origin = tmp;
		if (is_true)
			tmp |= value;
		else
			tmp &= ~value;
		if (origin != tmp)
			WREG32(reg_addr, tmp);
	}

	if (vf_access_select & AMDGV_VF_ACCESS_DOORBELL) {
		reg_addr = SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_VF_DOORBELL_EN);
		tmp = RREG32(reg_addr);
		origin = tmp;
		if (is_true)
			tmp |= value;
		else
			tmp &= ~value;
		if (origin != tmp)
			WREG32(reg_addr, tmp);
	}

	if (vf_access_select & AMDGV_VF_ACCESS_MMIO_REG_WRITE) {
		reg_addr = SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_VF_REGWR_EN);
		tmp = RREG32(reg_addr);
		origin = tmp;
		if (is_true)
			tmp |= value;
		else
			tmp &= ~value;
		if (origin != tmp)
			WREG32(reg_addr, tmp);
	}

	return 0;
}

static bool navi32_gpuiov_get_vf_access(struct amdgv_adapter *adapt, uint32_t idx_vf,
				      uint32_t vf_access_select)
{
	uint32_t tmp, reg_addr, value;

	if (idx_vf == AMDGV_PF_IDX)
		return false;
	if (AMDGV_IS_IDX_INVALID(idx_vf))
		return false;

	value = (uint32_t)0x1 << idx_vf;

	if (vf_access_select & AMDGV_VF_ACCESS_FB)
		reg_addr = SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_VF_FB_EN);
	else if (vf_access_select & AMDGV_VF_ACCESS_DOORBELL)
		reg_addr = SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_VF_DOORBELL_EN);
	else if (vf_access_select & AMDGV_VF_ACCESS_MMIO_REG_WRITE)
		reg_addr = SOC15_REG_OFFSET(NBIO, 0, regBIF_BX0_VF_REGWR_EN);
	else
		return false;

	tmp = RREG32(reg_addr);
	return tmp & value;
}

static int navi32_gpuiov_get_fb_info(struct amdgv_adapter *adapt)
{
	uint32_t offset;
	uint16_t total_fb_avail;
	uint64_t tom;

	/* read total available framebuffer */
	offset = adapt->gpuiov.pos + PCI_GPUIOV_TOTAL_FB_AVAILABLE;
	oss_pci_read_config_word(adapt->dev, offset, &total_fb_avail);
	adapt->gpuiov.total_fb_avail = total_fb_avail;
	/* In Navi the Total FB usable is the 1MB aligned address of the
	 * whole GPU manager TOM
	 */
	amdgv_memmgr_get_tom(&adapt->memmgr_gpu, &tom);
	/* align to 2MB since FFBM is 2MB align */
	tom = (tom + (0x2ULL << 20) - 1) & ~((0x2ULL << 20) - 1);
	adapt->gpuiov.total_fb_usable = total_fb_avail - (tom >> 20);

	AMDGV_INFO("Total FB Available = %d MB, Max usable FB size = %d MB\n", total_fb_avail,
		   adapt->gpuiov.total_fb_usable);

	/* compute CSA address (offset) to be sent to RLC_V and MMSCH */
	/* need to account for CSA located at TOP (end) of FrameBuffer
	 * o because using "adapt->memmgr_gpu" performs allocation from fb TOP,
	 * o "amdgv_memmgr_get_offset()" will return offset from fb TOP!
	 * o but RLC_V and MMSCH expect offset from fb BASE
	 */
	if (!adapt->gpuiov.csa_fb_mem) {
		AMDGV_ERROR("Private csa fb memory not allocated\n");
		return AMDGV_FAILURE;
	}
	adapt->gpuiov.resv_addr = ((uint64_t)total_fb_avail << 20) -
				  amdgv_memmgr_get_offset(adapt->gpuiov.csa_fb_mem);
	adapt->gpuiov.resv_size = amdgv_memmgr_get_size(adapt->gpuiov.csa_fb_mem);

	return 0;
}

static void navi32_gpuiov_get_sch_offset(struct amdgv_adapter *adapt)
{
	uint32_t offset;
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(navi32_hw_sched_static_config); i++) {

		/* JPEG does not have a GPUIOV block. Skip reading */
		if (i == NAVI32_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH)
			continue;

		offset = adapt->gpuiov.pos + navi32_hw_sched_static_config[i].pci_gpuiov_offset;
		oss_pci_read_config_byte(adapt->dev, offset, &adapt->gpuiov.ctrl_blocks[i].offset);

		AMDGV_INFO("pci offset = 0x%x %s block offset = 0x%x\n", offset,\
			adapt->gpuiov.ctrl_blocks[i].name,		\
			adapt->gpuiov.ctrl_blocks[i].offset << 4);
	}
}

static int navi32_gpuiov_get_config_info(struct amdgv_adapter *adapt)
{
	adapt->gpuiov.pos = navi32_gpuiov_find_cap(adapt);
	if (adapt->gpuiov.pos == 0)
		return AMDGV_FAILURE;

	if (navi32_gpuiov_get_fb_info(adapt))
		return AMDGV_FAILURE;

	navi32_gpuiov_get_sch_offset(adapt);

	return 0;
}

static int navi32_gpuiov_toggle_rlcg_vf_interface(struct amdgv_adapter *adapt, uint32_t idx_vf, bool enable)
{

	AMDGV_DEBUG("RLCG VF Interface is %s\n", enable ? "enabled" : "disabled");
	WREG32(SOC15_REG_OFFSET(GC, 0, regSCRATCH_REG0), 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regSCRATCH_REG1), 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regSCRATCH_REG2), 0);
	WREG32(SOC15_REG_OFFSET(GC, 0, regSCRATCH_REG3), 0);

	gc_v11_0_3_toggle_rlcg_vf_interface(adapt, enable);

	return 0;
}

static void navi32_gpuiov_ctx_empty_intr_control(struct amdgv_adapter *adapt, uint32_t hw_sched_id, bool enable)
{
	AMDGV_INFO("Context-empty interrupt gate from RLC %s\n", enable ? "enabled" : "disabled");
	if (!enable)
		WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPM_GENERAL_13), 0);
	else if (adapt->gpuiov.ctrl_blocks[hw_sched_id].sched_mode ==
	    AMDGV_SCHED_HYBRID_LIQUID_MODE)
		WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPM_GENERAL_13), 1);
	else if (adapt->gpuiov.ctrl_blocks[hw_sched_id].sched_mode ==
	    AMDGV_SCHED_LIQUID_MODE)
		WREG32(SOC15_REG_OFFSET(GC, 0, regRLC_GPM_GENERAL_13), 2);
	return;
}

static int navi32_gpuiov_transfer_vf_data(struct amdgv_adapter *adapt,
					  uint32_t hw_sched_id, uint32_t idx_vf,
					  bool export)
{
	uint32_t data, cmd, func_id, next_func_id;
	int offset;

	cmd = AMDGV_TRANSFER_VF_DATA;
	func_id = PCI_GPUIOV_FUNC_ID(idx_vf);

	/* TRANSFER_VF_DATA uses next_func_id for Import = 0 and Export = 1 */
	next_func_id = (uint32_t)export;

	offset = navi32_gpuiov_get_sched_block_offset(adapt, hw_sched_id);
	if (offset == AMDGV_FAILURE) {
		AMDGV_ERROR("Get wrong offset\n");
		return AMDGV_FAILURE;
	}

	offset += PCI_SCH_CMD_CONTROL;

	data = (cmd & 0x0F) | CMD_EXECUTE | (func_id << 8) | (next_func_id << 16);

	AMDGV_DEBUG("send offset 0x%x with command 0x%x\n", offset, data);
	oss_pci_write_config_dword(adapt->dev, offset, data);

	adapt->gpuiov.ctrl_blocks[hw_sched_id].last_cmd = cmd;
	adapt->gpuiov.ctrl_blocks[hw_sched_id].last_status = AMDGV_CMD_STATUS_PENDING_EXECUTE;

	return 0;
}

static const struct amdgv_gpuiov_funcs navi32_gpuiov_funcs = {
	.set_cmd = navi32_gpuiov_set_cmd,
	.is_cmd_complete = navi32_gpuiov_is_cmd_complete,
	.dump_gpuiov_cmd_status = navi32_dump_gpuiov_cmd_status,
	.set_total_fb_consumed = navi32_gpuiov_set_total_fb_consumed,
	.get_total_fb_consumed = navi32_gpuiov_get_total_fb_consumed,
	.set_vf_fb = navi32_gpuiov_set_vf_fb,
	.get_vf_fb = navi32_gpuiov_get_vf_fb,
	.get_vm_busy_status = navi32_gpuiov_get_vm_busy_status,
	.get_intr_bits = navi32_gpuiov_get_intr_bits,
	.set_intr_bits = navi32_gpuiov_set_intr_bits,
	.get_intr_status = navi32_gpuiov_get_intr_status,
	.get_hvvm_mbox_index = navi32_gpuiov_get_hvvm_mbox_index,
	.update_hvvm_mbox_index = navi32_gpuiov_update_hvvm_mbox_index,
	.rcv_hvvm_mbox_msg = navi32_gpuiov_rcv_hvvm_mbox_msg,
	.trn_hvvm_mbox_data = navi32_gpuiov_trn_hvvm_mbox_data,
	.set_hvvm_mbox_valid = navi32_gpuiov_set_hvvm_mbox_valid,
	.set_hvvm_mbox_ack = navi32_gpuiov_set_hvvm_mbox_ack,
	.get_hvvm_mbox_msg_valid = navi32_gpuiov_get_hvvm_mbox_msg_valid,
	.clear_intr_status = navi32_gpuiov_clear_intr_status,
	.get_active_vfs = navi32_gpuiov_get_active_vfs,
	.set_active_vfs = navi32_gpuiov_set_active_vfs,
	.add_active_vf = navi32_gpuiov_add_active_vf,
	.remove_active_vf = navi32_gpuiov_remove_active_vf,
	.get_active_vf_idx = navi32_gpuiov_get_active_vf_idx,
	.get_active_vf_status = navi32_gpuiov_get_active_vf_status,
	.wait_auto_sched_stop = navi32_gpuiov_wait_auto_sched_stop,
	.get_time_quanta_index = navi32_gpuiov_get_time_quanta_index,
	.set_time_quanta_index = navi32_gpuiov_set_time_quanta_index,
	.get_time_quanta_option = navi32_gpuiov_get_time_quanta_option,
	.set_time_quanta_option = navi32_gpuiov_set_time_quanta_option,
	.set_scheduler_config_descriptor = navi32_gpuiov_set_scheduler_config_descriptor,
	.set_vf_access = navi32_gpuiov_set_vf_access,
	.get_vf_access = navi32_gpuiov_get_vf_access,
	.get_config_info = navi32_gpuiov_get_config_info,
	.toggle_rlcg_vf_interface = navi32_gpuiov_toggle_rlcg_vf_interface,
	.set_event_notification = navi32_set_event_notification,
	.setup_sched_debug_log = navi32_gpuiov_setup_sched_debug_log,
	.cmd_to_name = navi32_gpuiov_cmd_to_name,
	.ctx_empty_intr_control = navi32_gpuiov_ctx_empty_intr_control,
	.transfer_vf_data = navi32_gpuiov_transfer_vf_data,
};

static int navi32_gpuiov_sw_init(struct amdgv_adapter *adapt)
{
	uint64_t csa_mem_size;
	uint64_t csa_mem_align;
	int ret = 0;
	uint32_t i = 0;

	adapt->rlcv_stamp_todo = false;
	adapt->rlcv_stamp_status = false;
	adapt->rlcv_stamp_count = -2;
	// Initializing to -2 is to make the data in right order. We should guarantee
	// the data is recorded from IDLE, so bypass the first two world switch loops.

	adapt->gpuiov.funcs = &navi32_gpuiov_funcs;

	/* According to SRIOV-architect, SIZE needs to report the
	 * (potentially changing) per-SOC per-VF CSA area requirements.
	 * HV was expected to read the SIZE and along with SR-IOV.NumVFs,
	 * determine the total CSA area that it would need to reserve
	 * (== NumVFs + 1, for PF), program a suitable CSA address location
	 * and this part of init was complete.
	 *
	 *
	 * since performing mem_allocation in sw_init, need readback SIZE
	 *  from VBIOS or PCI_CONFIG_SPACE (not implemented for navi1x)
	 *  it is currently (for navi10) been programmed by RLCV
	 */
	csa_mem_size = NAVI32_PCI_CONTEXT_SIZE * NAVI32_CSA_SIZE_PER_VF * (AMDGV_MAX_VF_NUM + 1);
	AMDGV_DEBUG("CSA_MEM_SIZE_RESERVED=%ld (pci_cntx_size=%d)\n", csa_mem_size,
		    NAVI32_PCI_CONTEXT_SIZE);
	/* CSA must be aligned to NAVI32_CSA_SIZE_PER_VF */
	csa_mem_align = NAVI32_CSA_SIZE_PER_VF;
	/* Reserve CSA from TOP of framebuffer (using memmgr_gpu) */
	adapt->gpuiov.csa_fb_mem =
		amdgv_memmgr_alloc_align(&adapt->memmgr_gpu, csa_mem_size, csa_mem_align,
					 MEM_GPUIOV_CSA);

	if (!adapt->gpuiov.csa_fb_mem) {
		AMDGV_ERROR("Failed to reserve memory for CSA\n");
		return AMDGV_FAILURE;
	}

	adapt->gpuiov.csa_max_vf_num = NAVI32_CSA_MAX_VF_NUM;

	if (!(adapt->flags & AMDGV_FLAG_USE_PF)) {
		/* enable config space FLR for KVM */
		adapt->flags |= AMDGV_FLAG_ENABLE_CFG_FLR_NOTIFY;
	}

	adapt->gpuiov.sched_cfg_mem =
		amdgv_memmgr_alloc_align(&adapt->memmgr_pf, SCHEDULER_DESCRIPTOR_SIZE * 4,
					PAGE_SIZE, MEM_GPUIOV_SCHED_CFG_DESC);
	if (!adapt->gpuiov.sched_cfg_mem) {
		AMDGV_ERROR("Failed to allocate debug dump memory!\n");
		return AMDGV_FAILURE;
	}
	AMDGV_DEBUG("SCHEDULER DESCRIPTOR MEM: GPU_ADDR=0x%llx MEM_ADDR=0x%llx MEM_SIZE=0x%llx\n",
			amdgv_memmgr_get_gpu_addr(adapt->gpuiov.sched_cfg_mem),
			amdgv_memmgr_get_offset(adapt->gpuiov.sched_cfg_mem),
			amdgv_memmgr_get_size(adapt->gpuiov.sched_cfg_mem));

	ret = amdgv_gpuiov_ctrl_block_setup(adapt, navi32_hw_sched_static_config, NAVI32_HW_SCHED_BLOCK_NUM);

	// Override the default GFX scheduler mode to manual switch if adapt->flags & AMDGV_FLAG_USE_PF
	if (adapt->flags & AMDGV_FLAG_USE_PF) {
		for (i = 0; i < adapt->gpuiov.num_ctrl_blocks; i++) {
			if (adapt->gpuiov.ctrl_blocks[i].hw_sched_type != AMDGV_HW_SCHED_TYPE_GFX)
				continue;
			if (!amdgv_gpuiov_is_sched_mode_supported(adapt, navi32_hw_sched_static_config[i], adapt->opt.gfx_sched_mode))
				adapt->gpuiov.ctrl_blocks[i].sched_mode = AMDGV_SCHED_FAIRNESS;
		}
	}

	return ret;
}

static int navi32_gpuiov_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->gpuiov.csa_fb_mem) {
		amdgv_memmgr_free(adapt->gpuiov.csa_fb_mem);
		adapt->gpuiov.csa_fb_mem = NULL;
	}

	if (adapt->gpuiov.sched_cfg_mem) {
		amdgv_memmgr_free(adapt->gpuiov.sched_cfg_mem);
		adapt->gpuiov.sched_cfg_mem = NULL;
	}

	if (adapt->gpuiov.debug_dump_mem) {
		amdgv_memmgr_free(adapt->gpuiov.debug_dump_mem);
		adapt->gpuiov.debug_dump_mem = NULL;
	}
	if (adapt->gpuiov.perf_log_mem) {
		amdgv_memmgr_free(adapt->gpuiov.perf_log_mem);
		adapt->gpuiov.perf_log_mem = NULL;
	}
	return 0;
}

static int navi32_gpuiov_hw_init(struct amdgv_adapter *adapt)
{
	int ret;
	uint32_t offset;
	uint32_t data;
	uint32_t cntx_offset;
	uint32_t cntx_loc;
	uint32_t cntx_size;
	uint32_t intr_enable;
	uint64_t csa_mem_size;
	uint32_t strap4;
	uint32_t hw_sched_id;

	adapt->gpuiov.pos = navi32_gpuiov_find_cap(adapt);
	if (adapt->gpuiov.pos == 0)
		return AMDGV_FAILURE;

	if (amdgv_gpuiov_init(adapt) < 0)
		return AMDGV_FAILURE;

	if (navi32_gpuiov_get_fb_info(adapt))
		return AMDGV_FAILURE;

	/* set the csa's offset, loc and size */
	/* NOTE: cntx_size field in GPUIOV is not writable from GIM;
	 *   only CNTXT_OFFSET[31:10] should be updated
	 *   Fields CNTXT_SIZE[6:0] and CNTXT_LOCATION[7] are read only,
	 * Note: SIZE & LOC are read-only fields.
	 *   SIZE == SOC-specific & denotes total amount of memory needed per
	 *    context (not really something we want the HV to be programming).
	 *   LOC == 0 == only support to FB (this bit has any real meaning)
	 * NOTE: MMSCH_GPUIOV_CNTXT_IP and RLC_RLCS_IOV_CNTX_LOC_SIZE should
	 *   reflect the same value in PCI_GPUIOV_CNTXT
	 */
	offset = adapt->gpuiov.pos + PCI_GPUIOV_CNTXT;
	oss_pci_read_config_dword(adapt->dev, offset, &data);
	cntx_offset =
		(data >> PCI_GPUIOV_CNTXT__OFFSET__SHIFT) & PCI_GPUIOV_CNTXT__OFFSET__MASK;
	cntx_loc = (data >> PCI_GPUIOV_CNTXT__LOC__SHIFT) & PCI_GPUIOV_CNTXT__LOC__MASK;
	cntx_size = (data >> PCI_GPUIOV_CNTXT__SIZE__SHIFT) & PCI_GPUIOV_CNTXT__SIZE__MASK;
	AMDGV_DEBUG("PCI_GPUIOV_CNTXT=0x%08x (readback) "
		    "CONTEXT_OFFSET[31:10]=0x%x LOC=0x%x CONTEXT_SIZE[6:0]=0x%x\n",
		    data, cntx_offset, cntx_loc, cntx_size);

	csa_mem_size = cntx_size * NAVI32_CSA_SIZE_PER_VF * (AMDGV_MAX_VF_NUM + 1);
	AMDGV_DEBUG("CSA_MEM_SIZE_REQUIRED=%ld (pci_cntx_size=%d)\n", csa_mem_size, cntx_size);

	/* According to SRIOV-architect,
	 * - GIM/LIBGV will program PCI_CONFIG_SPACE
	 * (for PCI_CONTEXT_OFFSET, PCI_CONTEXT_LOC, PCI_CONTEXT_SIZE)
	 *   o Value will multi-cast to:
	 *         MMSCH_GPUIOV_CNTXT,
	 *         RLC_RLCS_IOV_CNTX_LOC_SIZE,
	 *         RLC_GPU_IOV_CFG_REG6
	 *   o for navi1x, RLC and MMSCH should ignore LOC & SIZE fields
	 *         (and use ucode values instead).
	 */
	cntx_offset = TO_256KBYTES(adapt->gpuiov.resv_addr);
	data = (cntx_offset & PCI_GPUIOV_CNTXT__OFFSET__MASK)
	       << PCI_GPUIOV_CNTXT__OFFSET__SHIFT;
	data |= (cntx_loc & PCI_GPUIOV_CNTXT__LOC__MASK) << PCI_GPUIOV_CNTXT__LOC__SHIFT;
	data |= (cntx_size & PCI_GPUIOV_CNTXT__SIZE__MASK) << PCI_GPUIOV_CNTXT__SIZE__SHIFT;
	oss_pci_write_config_dword(adapt->dev, offset, data);
	AMDGV_DEBUG("PCI_GPUIOV_CNTXT=0x%08x (updated) "
		    "CONTEXT_OFFSET[31:10]=0x%x LOC=0x%x CONTEXT_SIZE[6:0]=0x%x\n",
		    data, cntx_offset, cntx_loc, cntx_size);

	/* enable msi-x vector 2 */
	offset = adapt->gpuiov.pos + PCI_GPUIOV_ENG_A_INTR_ENABLE;
	intr_enable = AMDGV_GFX_CMD_COMPLETE_INTR |
		      AMDGV_GFX_HANG_SELF_RECOVERED_INTR |
		      AMDGV_GFX_HANG_NEED_FLR_INTR |
		      AMDGV_GFX_VM_BUSY_TRANSITION_INTR;
	oss_pci_write_config_dword(adapt->dev, offset, intr_enable);

	offset = adapt->gpuiov.pos + PCI_GPUIOV_ENG_B_INTR_ENABLE;
	intr_enable = (NAVI32_JPEG_HANG_NEED_FLR_INTR |
		      AMDGV_UVD_HANG_SELF_RECOVERED_INTR |
		      AMDGV_UVD_HANG_NEED_FLR_INTR |
		      AMDGV_UVD_VM_BUSY_TRANSITION_INTR |
		      AMDGV_UVD1_CMD_COMPLETE_INTR |
		      AMDGV_UVD1_HANG_SELF_RECOVERED_INTR |
		      AMDGV_UVD1_HANG_NEED_FLR_INTR |
		      AMDGV_UVD1_VM_BUSY_TRANSITION_INTR) >> 8;
	oss_pci_write_config_dword(adapt->dev, offset, intr_enable);

	offset = adapt->gpuiov.pos + PCI_GPUIOV_INTR_ENABLE;
	intr_enable = AMDGV_HVVM_MAILBOX_TRN_ACK_INTR |
		      AMDGV_HVVM_MAILBOX_RCV_VALID_INTR;
	oss_pci_write_config_dword(adapt->dev, offset, intr_enable);

	/* enable config space FLR for KVM and one VF only */
	if (adapt->flags & AMDGV_FLAG_ENABLE_CFG_FLR_NOTIFY) {
		strap4 = RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP4));
		strap4 = REG_SET_FIELD(strap4, RCC_DEV0_EPF0_STRAP4, STRAP_FLR_EN_DEV0_F0, 1);
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP4), strap4);
	}

	navi32_gpuiov_get_sch_offset(adapt);

	if (!adapt->reset.reset_state) {
		/* enable sriov */
		ret = oss_pci_enable_sriov(adapt->dev, adapt->num_vf);
		if (ret < 0) {
			AMDGV_ERROR("oss_pci_enable_sriov(num_vf=%d) failed! "
				    "ret=%d\n",
				    adapt->num_vf, ret);
			return AMDGV_FAILURE;
		}
		AMDGV_INFO("PCI_ENABLE_SRIOV(num_vf=%d)\n", adapt->num_vf);
		/* readback cntx_size for csa offset calculation */
		offset = adapt->gpuiov.pos + PCI_GPUIOV_CNTXT;
		oss_pci_read_config_dword(adapt->dev, offset, &data);
		cntx_size = (data >> PCI_GPUIOV_CNTXT__SIZE__SHIFT) & PCI_GPUIOV_CNTXT__SIZE__MASK;
		adapt->gpuiov.csa_size_per_vf = cntx_size * NAVI32_CSA_SIZE_PER_VF;
	} else {
		amdgv_reset_restore_sriov(adapt);
	}

	for (hw_sched_id = 0; hw_sched_id < adapt->gpuiov.num_ctrl_blocks; hw_sched_id++) {
		if (adapt->gpuiov.ctrl_blocks[hw_sched_id].hw_sched_type == AMDGV_HW_SCHED_TYPE_GFX &&
			adapt->gpuiov.ctrl_blocks[hw_sched_id].sched_mode == AMDGV_SCHED_LIQUID_MODE)
			navi32_gpuiov_ctx_empty_intr_control(adapt, hw_sched_id, true);
	}

	return 0;
}

static int navi32_gpuiov_hw_fini(struct amdgv_adapter *adapt)
{
	uint32_t i;
	uint32_t strap4;

	for (i = 0; i < adapt->gpuiov.num_ctrl_blocks; i++) {
		if (adapt->gpuiov.ctrl_blocks[i].hw_sched_type == AMDGV_HW_SCHED_TYPE_GFX &&
			adapt->gpuiov.ctrl_blocks[i].sched_mode == AMDGV_SCHED_LIQUID_MODE)
			navi32_gpuiov_ctx_empty_intr_control(adapt, i, false);
	}

	/* disable config space FLR for KVM and one VF only */
	if (adapt->flags & AMDGV_FLAG_ENABLE_CFG_FLR_NOTIFY) {
		strap4 = RREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP4));
		strap4 = REG_SET_FIELD(strap4, RCC_DEV0_EPF0_STRAP4, STRAP_FLR_EN_DEV0_F0, 0);
		WREG32(SOC15_REG_OFFSET(NBIO, 0, regRCC_STRAP0_RCC_DEV0_EPF0_STRAP4), strap4);
	}

	if (!adapt->reset.reset_state) {
		/* amdgv_sched_stop() before disable SRIOV
		 *  need to stop sending GPUIOV commands to RLCV/MMSCH
		 *  => Therefore, need to stop all world-switching
		 */
		amdgv_sched_stop_all(adapt);
		oss_pci_disable_sriov(adapt->dev);

		amdgv_gpuiov_fini(adapt);
	} else {
		for (i = 0; i < adapt->num_vf; i++) {
			amdgv_gpuiov_set_vf_fb(adapt, i, 0, 0);
		}

		oss_pci_write_config_dword(adapt->dev,
					adapt->sriov_cap_pos + PCIE_EXT_SRIOV_CTRL, 0);
		/* wait after sriov disablement for SMU to recover */
		if (adapt->pp.pp_funcs->wait_smu_idle) {
			/*try at leasst NAVI32_WAIT_SMU_IDLE_MAX_RETRY times before go ahead with the sequence*/
			for (i = 0; i < NAVI32_WAIT_SMU_IDLE_MAX_RETRY; i++) {
				if (adapt->pp.pp_funcs->wait_smu_idle(adapt, 1) == 0)
					return 0;
			}
			return AMDGV_FAILURE;
		}
	}

	return 0;
}

static int navi32_gpuiov_post_reset(struct amdgv_adapter *adapt)
{
	uint32_t i;

	for (i = 0; i < adapt->num_vf; i++) {
		amdgv_gpuiov_set_vf_fb(adapt, i, 0, 0);
	}

	oss_pci_write_config_dword(adapt->dev,
		 adapt->sriov_cap_pos + PCIE_EXT_SRIOV_CTRL, 0);

	return 0;
}

struct amdgv_init_func navi32_gpuiov_func = {
	.name = "navi32_gpuiov_func",
	.sw_init = navi32_gpuiov_sw_init,
	.sw_fini = navi32_gpuiov_sw_fini,
	.hw_init = navi32_gpuiov_hw_init,
	.hw_fini = navi32_gpuiov_hw_fini,
	.post_reset = navi32_gpuiov_post_reset,
};
