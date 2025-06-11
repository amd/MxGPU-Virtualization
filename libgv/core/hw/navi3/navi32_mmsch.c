/*
 * Copyright (C) 2021  Advanced Micro Devices, Inc.
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
#include <amdgv_sched.h>
#include <amdgv_sched_internal.h>

#include <amdgv_oss.h>
#include <amdgv_oss_wrapper.h>

#include <amdgv_mmsch.h>
#include "navi32_mmsch.h"
#include "navi32_reg_inc.h"
#include "navi32_smu_ppsmc_wrapper.h"
// #include "navi32_powerplay.h"
#include "navi32_powerplay.h"

static const uint32_t this_block = AMDGV_MULTIMEDIA_BLOCK;

static int navi32_mmsch_send_mailbox(struct amdgv_adapter *adapt,
				uint32_t cmd, uint32_t data)
{
	int wait_ret;

	uint32_t reg_resp_data;
	uint32_t reg_resp_offset = SOC15_REG_OFFSET(VCN, 0, regMMSCH_VF_MAILBOX_1_RESP);

	WREG32(reg_resp_offset, 0);
	WREG32(SOC15_REG_OFFSET(VCN, 0, regMMSCH_VF_MAILBOX1_DATA), data);
	WREG32(SOC15_REG_OFFSET(VCN, 0, regMMSCH_VF_MAILBOX_1), cmd);

	wait_ret = amdgv_wait_for_register(
		adapt, reg_resp_offset,
		0xFFFFFFFF, MMSCH_HV_RESP__DONE,
		AMDGV_TIMEOUT(TIMEOUT_CMD_RESP), AMDGV_WAIT_CHECK_EQ,
		AMDGV_WAIT_FLAG_AUTO
	);

	if (wait_ret) {
		AMDGV_ERROR("MMSCH failed to response to mailbox data=0x%x cmd=0x%x resp=0x%x\n",
			data, cmd, RREG32(reg_resp_offset));
		return AMDGV_FAILURE;
	}

	reg_resp_data = RREG32(reg_resp_offset);
	if (reg_resp_data == 0xFFFFFFFF) {
		AMDGV_ERROR("MMSCH mailbox response register is protected and cannot be read\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int navi32_mmsch_notify_mmsch(struct amdgv_adapter *adapt)
{
	return navi32_mmsch_send_mailbox(adapt, MMSCH_HV_CMD__NEW_CMD_READY, 0);
}

static int navi32_mmsch_set_enabled_vf(struct amdgv_adapter *adapt)
{
	struct amdgv_mmsch_bandwidth_config *sw_bw_cfg;
	struct navi32_mmsch_bandwidth_config *hw_bw_cfg;

	uint32_t vcn_engine_idx;

	sw_bw_cfg = &adapt->mmsch.bandwidth_config;
	hw_bw_cfg = (struct navi32_mmsch_bandwidth_config *)amdgv_memmgr_get_cpu_addr(adapt->mmsch.bandwidth_config_mem);

	for (vcn_engine_idx = 0; vcn_engine_idx < NAVI3_MMSCH_MAX_VCN_ENGINE; vcn_engine_idx++)
		hw_bw_cfg->vcn_enabled_vfs[vcn_engine_idx] = sw_bw_cfg->vcn_enabled_vfs[vcn_engine_idx];

	return navi32_mmsch_notify_mmsch(adapt);
}

static int navi32_mmsch_get_default_bandwidth_config(struct amdgv_adapter *adapt,
				struct amdgv_mmsch_bandwidth_config *bw_cfg)
{
	uint32_t vcn_engine_idx;
	uint32_t mmsch_idx_vf;
	uint32_t vf_enable_bit;
	uint32_t pf_enable_bit;
	struct amdgv_sched_world_switch *world_switch;

	struct amdgv_mmsch_vcn_vf_bandwidth *vf_bw;

	if (!bw_cfg)
		return AMDGV_FAILURE;

	bw_cfg->flags.time_partition_enable = 1;
	bw_cfg->flags.job_limit_enable = 1;

	for (vcn_engine_idx = 0; vcn_engine_idx < NAVI3_MMSCH_MAX_VCN_ENGINE; vcn_engine_idx++) {
		if (amdgv_mmsch_vcn_engine_idx_to_world_switch(adapt, vcn_engine_idx, &world_switch)) {
			return AMDGV_FAILURE;
		}

		/* set MMSCH's PF/VF enable bit (PF is bit0 here) */
		vf_enable_bit = world_switch->allowed_vf_assignment & ((1 << adapt->num_vf) - 1);
		vf_enable_bit <<= 1;
		pf_enable_bit = world_switch->allowed_vf_assignment >> 31;
		bw_cfg->vcn_enabled_vfs[vcn_engine_idx] = vf_enable_bit | pf_enable_bit;

		/* set bandwidth config */
		for (mmsch_idx_vf = 0; mmsch_idx_vf < NAVI3_MMSCH_MAX_VF_SLOT; mmsch_idx_vf++) {
			vf_bw = &bw_cfg->vcn_vf_bandwidth[vcn_engine_idx][mmsch_idx_vf];

			/* only set enabled VF config, leave the other data empty */
			if (bw_cfg->vcn_enabled_vfs[vcn_engine_idx] & (1 << mmsch_idx_vf)) {
				vf_bw->partition_usecs = 256000;

				if (adapt->num_vf == 1) {
					vf_bw->job_limit.encode_max_dimension_pixels = 7680;
					vf_bw->job_limit.encode_max_frame_pixels     = 7680 * 4352;
					vf_bw->job_limit.decode_max_dimension_pixels = 7680;
					vf_bw->job_limit.decode_max_frame_pixels     = 7680 * 4352;
				} else if (adapt->num_vf <= 4) {
					vf_bw->job_limit.encode_max_dimension_pixels = 3840;
					vf_bw->job_limit.encode_max_frame_pixels     = 3840 * 2176;
					vf_bw->job_limit.decode_max_dimension_pixels = 3840;
					vf_bw->job_limit.decode_max_frame_pixels     = 3840 * 2176;
				} else {
					vf_bw->job_limit.encode_max_dimension_pixels = 2560;
					vf_bw->job_limit.encode_max_frame_pixels     = 2560 * 1472;
					vf_bw->job_limit.decode_max_dimension_pixels = 2560;
					vf_bw->job_limit.decode_max_frame_pixels     = 2560 * 1472;
				}
			} else {
				oss_memset(vf_bw, 0, sizeof(struct amdgv_mmsch_vcn_vf_bandwidth));
			}
		}
	}

	return 0;
}

static int navi32_mmsch_read_bandwidth_config(struct amdgv_adapter *adapt)
{
	struct amdgv_mmsch_bandwidth_config *sw_bw_cfg;
	struct navi32_mmsch_bandwidth_config *hw_bw_cfg;

	uint32_t vcn_engine_idx;

	sw_bw_cfg = &adapt->mmsch.bandwidth_config;
	hw_bw_cfg = (struct navi32_mmsch_bandwidth_config *)amdgv_memmgr_get_cpu_addr(adapt->mmsch.bandwidth_config_mem);

	if (!hw_bw_cfg)
		return AMDGV_FAILURE;

	/* copy bandwidth config output from framebuffer to software struct */
	for (vcn_engine_idx = 0; vcn_engine_idx < NAVI3_MMSCH_MAX_VCN_ENGINE; vcn_engine_idx++) {
		oss_memcpy(
			&sw_bw_cfg->vcn_vf_status[vcn_engine_idx],
			&hw_bw_cfg->vcn_vf_status[vcn_engine_idx],
			NAVI3_MMSCH_MAX_VF_SLOT * sizeof(struct amdgv_mmsch_vcn_vf_status));
	}

	sw_bw_cfg->output_gpu_timestamp_hi = hw_bw_cfg->output_gpu_timestamp_hi;
	sw_bw_cfg->output_gpu_timestamp_lo = hw_bw_cfg->output_gpu_timestamp_lo;

	return 0;
}

static int navi32_mmsch_write_bandwidth_config(struct amdgv_adapter *adapt, uint32_t mmsch_idx_vf)
{
	struct amdgv_mmsch_bandwidth_config *sw_bw_cfg;
	struct navi32_mmsch_bandwidth_config *hw_bw_cfg;

	uint32_t vcn_engine_idx;

	sw_bw_cfg = &adapt->mmsch.bandwidth_config;
	hw_bw_cfg = (struct navi32_mmsch_bandwidth_config *)amdgv_memmgr_get_cpu_addr(adapt->mmsch.bandwidth_config_mem);

	if (!hw_bw_cfg)
		return AMDGV_FAILURE;

	/* copy bandwidth config input from software struct to framebuffer */
	hw_bw_cfg->flags.allbits = sw_bw_cfg->flags.allbits;
	for (vcn_engine_idx = 0; vcn_engine_idx < NAVI3_MMSCH_MAX_VCN_ENGINE; vcn_engine_idx++) {
		/* only write data if the VF is enabled, else zero out the config */
		if (sw_bw_cfg->vcn_enabled_vfs[vcn_engine_idx] & (1 << mmsch_idx_vf)) {
			oss_memcpy(
				&hw_bw_cfg->vcn_vf_bandwidth[vcn_engine_idx][mmsch_idx_vf],
				&sw_bw_cfg->vcn_vf_bandwidth[vcn_engine_idx][mmsch_idx_vf],
				sizeof(struct amdgv_mmsch_vcn_vf_bandwidth));
		} else {
			oss_memset(
				&hw_bw_cfg->vcn_vf_bandwidth[vcn_engine_idx][mmsch_idx_vf],
				0,
				sizeof(struct amdgv_mmsch_vcn_vf_bandwidth));
		}
	}

	return 0;
}

static int navi32_mmsch_clear_bandwidth_config_vf_status(struct amdgv_adapter *adapt, uint32_t mmsch_idx_vf)
{
	struct amdgv_mmsch_bandwidth_config *sw_bw_cfg;
	struct navi32_mmsch_bandwidth_config *hw_bw_cfg;

	uint32_t vcn_engine_idx;

	sw_bw_cfg = &adapt->mmsch.bandwidth_config;
	hw_bw_cfg = (struct navi32_mmsch_bandwidth_config *)amdgv_memmgr_get_cpu_addr(adapt->mmsch.bandwidth_config_mem);

	if (!hw_bw_cfg)
		return AMDGV_FAILURE;

	/* clear VF status bits for re-init */
	for (vcn_engine_idx = 0; vcn_engine_idx < NAVI3_MMSCH_MAX_VCN_ENGINE; vcn_engine_idx++) {
		sw_bw_cfg->vcn_vf_status[vcn_engine_idx][mmsch_idx_vf].allbits = 0;
		hw_bw_cfg->vcn_vf_status[vcn_engine_idx][mmsch_idx_vf].allbits = 0;
	}

	return 0;
}

static int navi32_mmsch_check_enabled_features(struct amdgv_adapter *adapt)
{
	/* Always enabled on NV3x */
	adapt->mmsch.is_feature_enabled.cmd_buffer = true;

	/* SHUTDOWN_GPU command is only supported after navi32 MMSCH 7.0.45 (include) */
	if (adapt->psp.fw_info[AMDGV_FIRMWARE_ID__MMSCH] >= 0x0700002d)
		adapt->mmsch.is_feature_enabled.support_shutdown_cmd = true;

	return 0;
}

int navi32_mmsch_modify_vcn_ip_discovery_revison(struct amdgv_adapter *adapt, uint32_t libgv_idx_vf,
				uint32_t vcn_engine_idx, uint8_t *revision)
{
	struct amdgv_mmsch_bandwidth_config *sw_bw_cfg;

	uint32_t mmsch_idx_vf;

	sw_bw_cfg = &adapt->mmsch.bandwidth_config;
	mmsch_idx_vf = CONVERT_TO_MMSCH_IDX_VF(libgv_idx_vf);

	if (vcn_engine_idx >= adapt->mmsch.max_vcn_engine) {
		AMDGV_ERROR("VCN engine index is out of range\n");
		return AMDGV_FAILURE;
	}

	/*
	 * modify VCN revision (uint8)
	 * Bit [5:0]: original revision value
	 * Bit [7:6]: en/decode capability:
	 *     0b00 : VCN function normally
	 *     0b10 : encode is disabled
	 *     0b01 : decode is disabled
	 */
	if (!(sw_bw_cfg->vcn_enabled_vfs[vcn_engine_idx] & (1 << mmsch_idx_vf))) {
		*revision |= NAVI3_MMSCH_VCN_BLOCK_ENCODE_DISABLE_BIT | NAVI3_MMSCH_VCN_BLOCK_DECODE_DISABLE_BIT;
	}

	return 0;
}

static int navi32_mmsch_write_rb_decouple(struct amdgv_adapter *adapt)
{
	struct amdgv_mmsch_rb_decouple *sw_rb_decouple;
	struct amdgv_mmsch_rb_decouple *hw_rb_decouple;

	sw_rb_decouple = &adapt->mmsch.rb_decouple;
	hw_rb_decouple = (struct amdgv_mmsch_rb_decouple *)amdgv_memmgr_get_cpu_addr(adapt->mmsch.rb_decouple_mem);

	if (!hw_rb_decouple)
		return AMDGV_FAILURE;

	oss_memcpy(hw_rb_decouple, sw_rb_decouple, sizeof(struct amdgv_mmsch_rb_decouple));

	return 0;
}

const struct amdgv_mmsch_funcs navi32_mmsch_funcs = {
	.check_enabled_features           = navi32_mmsch_check_enabled_features,
	.send_mailbox                     = navi32_mmsch_send_mailbox,
	.notify_mmsch                     = navi32_mmsch_notify_mmsch,
	.get_default_bandwidth_config     = navi32_mmsch_get_default_bandwidth_config,
	.read_bandwidth_config            = navi32_mmsch_read_bandwidth_config,
	.write_bandwidth_config           = navi32_mmsch_write_bandwidth_config,
	.clear_bandwidth_config_vf_status = navi32_mmsch_clear_bandwidth_config_vf_status,
	.write_rb_decouple                = navi32_mmsch_write_rb_decouple,
};

static int navi32_mmsch_sw_init(struct amdgv_adapter *adapt)
{
	uint32_t pf_enable_bit;

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

	adapt->mmsch.bandwidth_config_mem = amdgv_memmgr_alloc_align(
		&adapt->memmgr_pf,
		sizeof(struct navi32_mmsch_bandwidth_config),
		AMDGV_MMSCH_FB_ALIGNMENT,
		MEM_MMSCH_BW_CFG);

	if (!adapt->mmsch.bandwidth_config_mem) {
		AMDGV_ERROR("Failed to reserve memory for MMSCH bandwidth config\n");
		goto free;
	}

	adapt->mmsch.rb_decouple_mem = amdgv_memmgr_alloc_align(
		&adapt->memmgr_pf,
		sizeof(struct amdgv_mmsch_rb_decouple),
		AMDGV_MMSCH_FB_ALIGNMENT,
		MEM_MMSCH_RB_DECOUPLE);

	if (!adapt->mmsch.rb_decouple_mem) {
		AMDGV_ERROR("Failed to reserve memory for MMSCH RB decouple parameters\n");
		goto free;
	}

	adapt->mmsch.mmsch_funcs = &navi32_mmsch_funcs;

	adapt->mmsch.max_vcn_engine = NAVI3_MMSCH_MAX_VCN_ENGINE;
	adapt->mmsch.bandwidth_config_size = sizeof(struct navi32_mmsch_bandwidth_config);

	// set RB decouple with MMSCH index
	pf_enable_bit = NAVI3_MMSCH_RB_DECOUPLE_ENABLED_VF >> 31;
	adapt->mmsch.default_rb_decouple_enabled_vfs = (NAVI3_MMSCH_RB_DECOUPLE_ENABLED_VF << 1) | pf_enable_bit;
	pf_enable_bit = NAVI3_MMSCH_RB_DECOUPLE_AV1_DISABLED >> 31;
	adapt->mmsch.default_rb_decouple_av1_disabled = (NAVI3_MMSCH_RB_DECOUPLE_AV1_DISABLED << 1) | pf_enable_bit;

	adapt->mmsch.rb_decouple.enabled_vfs = adapt->mmsch.default_rb_decouple_enabled_vfs;
	adapt->mmsch.rb_decouple.av1_disabled = adapt->mmsch.default_rb_decouple_av1_disabled;

	return 0;

free:
	if (adapt->mmsch.cmd_buffer_mem)
		amdgv_memmgr_free(adapt->mmsch.cmd_buffer_mem);
	if (adapt->mmsch.sram_dump_mem)
		amdgv_memmgr_free(adapt->mmsch.sram_dump_mem);
	if (adapt->mmsch.bandwidth_config_mem)
		amdgv_memmgr_free(adapt->mmsch.bandwidth_config_mem);
	if (adapt->mmsch.rb_decouple_mem)
		amdgv_memmgr_free(adapt->mmsch.rb_decouple_mem);

	oss_memset(&adapt->mmsch, 0, sizeof(struct amdgv_mmsch));

	return AMDGV_FAILURE;
}

static int navi32_mmsch_sw_fini(struct amdgv_adapter *adapt)
{
	if (adapt->mmsch.cmd_buffer_mem)
		amdgv_memmgr_free(adapt->mmsch.cmd_buffer_mem);
	if (adapt->mmsch.sram_dump_mem)
		amdgv_memmgr_free(adapt->mmsch.sram_dump_mem);
	if (adapt->mmsch.bandwidth_config_mem)
		amdgv_memmgr_free(adapt->mmsch.bandwidth_config_mem);
	if (adapt->mmsch.rb_decouple_mem)
		amdgv_memmgr_free(adapt->mmsch.rb_decouple_mem);

	oss_memset(&adapt->mmsch, 0, sizeof(struct amdgv_mmsch));

	return 0;
}

static int navi32_mmsch_hw_init(struct amdgv_adapter *adapt)
{
	int ret;
	uint64_t mmsch_cmd_buff_mem;
	uint32_t *ptr;
	enum amdgv_firmware_id ucode_id;

	ret = navi32_power_on_vcn(adapt);
	if (ret)
		return ret;

	ucode_id = AMDGV_FIRMWARE_ID__MMSCH;
	ret = adapt->ucode.load(adapt, &ucode_id, 1);
	if (ret) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_FW_UCODE_LOAD_FAIL,
				ucode_id);
		return ret;
	}

	navi32_mmsch_check_enabled_features(adapt);

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

	/* MUST zero out framebuffer, or there may be residue data from previous driver loading */
	ptr = (uint32_t *)amdgv_memmgr_get_cpu_addr(adapt->mmsch.bandwidth_config_mem);
	oss_memset(ptr, 0, adapt->mmsch.bandwidth_config_size);

	ptr = (uint32_t *)amdgv_memmgr_get_cpu_addr(adapt->mmsch.rb_decouple_mem);
	oss_memset(ptr, 0, sizeof(struct amdgv_mmsch_rb_decouple));

	/* Populate default bandwidth config */
	amdgv_mmsch_get_default_bandwidth_config(adapt, &adapt->mmsch.bandwidth_config);

	/* write default enabled VF to framebuffer
	 * NOTE:
	 * (1) enabled VF cannot be changed after host driver init
	 * (2) since navi21, we want to support 2 VCNs in 1VF mode, therefore, the 0th bit of VCN1 must be set
	 *     however, it is not necessary to save this bit in sw_config since BW mgr can block VCN1 with multi-vf
	 */
	ret = navi32_mmsch_set_enabled_vf(adapt);
	if (ret) {
		AMDGV_ERROR("Failed to set enabled VF to MMSCH\n");
		adapt->mmsch.is_feature_enabled.cmd_buffer = false;
	}

	return ret;
}

static int navi32_mmsch_hw_fini(struct amdgv_adapter *adapt)
{
	int ret;

	ret = navi32_power_down_vcn(adapt);

	return ret;
}

const struct amdgv_init_func navi32_mmsch_func = {
	.name = "navi32_mmsch_func",
	.sw_init = navi32_mmsch_sw_init,
	.sw_fini = navi32_mmsch_sw_fini,
	.hw_init = navi32_mmsch_hw_init,
	.hw_fini = navi32_mmsch_hw_fini,
};
