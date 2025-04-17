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
 * THE SOFTWARE.
 */

#include <amdgv_device.h>
#include <amdgv_mmsch.h>
#include <amdgv_sched.h>
#include <amdgv_sched_internal.h>
#include <amdgv_vfmgr.h>

#include <amdgv_oss.h>
#include <amdgv_oss_wrapper.h>

static const uint32_t this_block = AMDGV_MULTIMEDIA_BLOCK;

/*
 * TODO:
 *  There should not be special definitions for MM HW schedulers.
 *  For Legacy ASICs, we can map logical sched to engine.
 *  For future ASICs, use the hw_sched_id directly.
 */
int amdgv_mmsch_vcn_engine_idx_to_world_switch(struct amdgv_adapter *adapt,
					       uint32_t vcn_engine_idx,
					       struct amdgv_sched_world_switch **world_switch)
{
	enum amdgv_sched_block sched_block;
	uint32_t i;

	*world_switch = NULL;

	switch (vcn_engine_idx) {
	case 0:
		sched_block = AMDGV_SCHED_BLOCK_VCN;
		break;
	case 1:
		sched_block = AMDGV_SCHED_BLOCK_VCN1;
		break;
	case 2:
		sched_block = AMDGV_SCHED_BLOCK_JPEG;
		break;
	default:
		AMDGV_ERROR("Invalid MM engine index\n");
		return AMDGV_FAILURE;
	}

	for (i = 0; i < adapt->sched.num_world_switch; i++) {
		if (adapt->sched.world_switch[i].sched_block == sched_block)
			*world_switch = &adapt->sched.world_switch[i];
	}

	if (!(*world_switch))
		return AMDGV_FAILURE;
	else
		return 0;
}

/*
 * TODO:
 *  There should not be special definitions for MM HW schedulers.
 *  For Legacy ASICs, we can map logical sched to engine.
 *  For future ASICs, use the hw_sched_id directly.
 */
int amdgv_mmsch_sched_block_to_vcn_engine_idx(struct amdgv_adapter *adapt,
					      enum amdgv_sched_block sched_block,
					      uint32_t *vcn_engine_idx)
{
	switch (sched_block) {
	case AMDGV_SCHED_BLOCK_VCN:
		*vcn_engine_idx = 0;
		break;
	case AMDGV_SCHED_BLOCK_VCN1:
		*vcn_engine_idx = 1;
		break;
	case AMDGV_SCHED_BLOCK_JPEG:
		*vcn_engine_idx = 2;
		break;
	default:
		AMDGV_ERROR("Invalid schedule block ID\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

int amdgv_mmsch_set_cmd_buffer_address(struct amdgv_adapter *adapt, uint64_t cmd_gpu_addr)
{
	int ret = 0;
	uint32_t data;

	/* if not supported, return 0 */
	if (!adapt->mmsch.is_feature_enabled.cmd_buffer)
		return 0;

	data = upper_32_bits(cmd_gpu_addr);
	ret = adapt->mmsch.mmsch_funcs->send_mailbox(adapt, MMSCH_HV_CMD__SET_CMD_BUFFER_ADDR_HI, data);
	if (ret)
		return ret;

	data = lower_32_bits(cmd_gpu_addr);
	ret = adapt->mmsch.mmsch_funcs->send_mailbox(adapt, MMSCH_HV_CMD__SET_CMD_BUFFER_ADDR_LO, data);
	return ret;
}

int amdgv_mmsch_submit_cmd(struct amdgv_adapter *adapt, enum amdgv_mmsch_cmd_buffer_type type,
			   uint32_t libgv_idx_vf)
{
	struct amdgv_memmgr_mem *cmd_buffer_mem = adapt->mmsch.cmd_buffer_mem;
	struct amdgv_mmsch_cmd_buffer *cmd_buffer_cpu_addr;

	struct amdgv_memmgr_mem *parameter_mem;
	uint64_t parameter_gpu_addr;

	uint32_t mmsch_idx_vf;

	/* if not supported, return 0 */
	if (!adapt->mmsch.is_feature_enabled.cmd_buffer)
		return 0;

	if (AMDGV_IS_IDX_INVALID(libgv_idx_vf))
		return AMDGV_FAILURE;

	if (!cmd_buffer_mem) {
		AMDGV_ERROR("MMSCH's command buffer is not allocated\n");
		return AMDGV_FAILURE;
	}

	cmd_buffer_cpu_addr = (struct amdgv_mmsch_cmd_buffer *)amdgv_memmgr_get_cpu_addr(cmd_buffer_mem);
	if (!cmd_buffer_cpu_addr) {
		AMDGV_ERROR("MMSCH's command buffer is not memory mapped?!\n");
		return AMDGV_FAILURE;
	}

	/* set command buffer type */
	cmd_buffer_cpu_addr->cmd_type = type;

	/* fill command context to command buffer according to type */
	switch (type) {
	case AMDGV_MMSCH_CMD_TYPE_BANDWIDTH_CONFIG:
		mmsch_idx_vf = CONVERT_TO_MMSCH_IDX_VF(libgv_idx_vf);

		parameter_mem = adapt->mmsch.bandwidth_config_mem;
		if (!parameter_mem) {
			AMDGV_ERROR("bandwidth config framebuffer is not allocated\n");
			return AMDGV_FAILURE;
		}

		parameter_gpu_addr = amdgv_memmgr_get_gpu_addr(parameter_mem);
		cmd_buffer_cpu_addr->cmd_info_addr_hi = upper_32_bits(parameter_gpu_addr);
		cmd_buffer_cpu_addr->cmd_info_addr_lo = lower_32_bits(parameter_gpu_addr);
		cmd_buffer_cpu_addr->cmd_info_size = adapt->mmsch.bandwidth_config_size;
		cmd_buffer_cpu_addr->cmd_status = 0;

		if (adapt->mmsch.mmsch_funcs->write_bandwidth_config(adapt, mmsch_idx_vf))
			return AMDGV_FAILURE;

		break;

	case AMDGV_MMSCH_CMD_TYPE_RB_DECOUPLE:
		// if func ptr does not exist, this ASIC does not support this function
		if (!adapt->mmsch.mmsch_funcs->write_rb_decouple)
			return 0;

		parameter_mem = adapt->mmsch.rb_decouple_mem;
		if (!parameter_mem) {
			AMDGV_ERROR("rb decouple framebuffer is not allocated\n");
			return AMDGV_FAILURE;
		}
		parameter_gpu_addr = amdgv_memmgr_get_gpu_addr(parameter_mem);
		cmd_buffer_cpu_addr->cmd_info_addr_hi = upper_32_bits(parameter_gpu_addr);
		cmd_buffer_cpu_addr->cmd_info_addr_lo = lower_32_bits(parameter_gpu_addr);
		cmd_buffer_cpu_addr->cmd_info_size = sizeof(struct amdgv_mmsch_rb_decouple);
		cmd_buffer_cpu_addr->cmd_status = 0;

		if (adapt->mmsch.mmsch_funcs->write_rb_decouple(adapt))
			return AMDGV_FAILURE;

		break;

	case AMDGV_MMSCH_CMD_TYPE_SRAM_DUMP:
		parameter_mem = adapt->mmsch.sram_dump_mem;
		if (!parameter_mem) {
			AMDGV_ERROR("sram dump framebuffer is not allocated\n");
			return AMDGV_FAILURE;
		}

		parameter_gpu_addr = amdgv_memmgr_get_gpu_addr(parameter_mem);
		cmd_buffer_cpu_addr->cmd_info_addr_hi = upper_32_bits(parameter_gpu_addr);
		cmd_buffer_cpu_addr->cmd_info_addr_lo = lower_32_bits(parameter_gpu_addr);
		cmd_buffer_cpu_addr->cmd_info_size = AMDGV_MMSCH_SRAM_DUMP_SIZE;
		cmd_buffer_cpu_addr->cmd_status = 0;

		break;

	default:
		AMDGV_ERROR("Invalid MMSCH command buffer type\n");
		return AMDGV_FAILURE;
	}

	amdgv_misc_hdp_flush(adapt);

	/* send interrupt to MMSCH to submit the command */
	return adapt->mmsch.mmsch_funcs->notify_mmsch(adapt);
}

int amdgv_mmsch_get_default_bandwidth_config(struct amdgv_adapter *adapt,
					     struct amdgv_mmsch_bandwidth_config *bw_cfg)
{
	/* if not supported, return 0 */
	if (!adapt->mmsch.is_feature_enabled.cmd_buffer)
		return 0;

	if (!bw_cfg) {
		AMDGV_ERROR("Output struct of BW config is not allocated\n");
		return AMDGV_FAILURE;
	}

	return adapt->mmsch.mmsch_funcs->get_default_bandwidth_config(adapt, bw_cfg);
}

int amdgv_mmsch_read_bandwidth_config(struct amdgv_adapter *adapt)
{
	/* if not supported, return 0 */
	if (!adapt->mmsch.is_feature_enabled.cmd_buffer)
		return 0;

	return adapt->mmsch.mmsch_funcs->read_bandwidth_config(adapt);
}

int amdgv_mmsch_clear_bandwidth_config_vf_status(struct amdgv_adapter *adapt,
						 uint32_t libgv_idx_vf)
{
	uint32_t mmsch_idx_vf;

	/* if not supported, return 0 */
	if (!adapt->mmsch.is_feature_enabled.cmd_buffer)
		return 0;

	if (AMDGV_IS_IDX_INVALID(libgv_idx_vf)) {
		AMDGV_ERROR("Invalid VF index\n");
		return AMDGV_FAILURE;
	}

	mmsch_idx_vf = CONVERT_TO_MMSCH_IDX_VF(libgv_idx_vf);

	return adapt->mmsch.mmsch_funcs->clear_bandwidth_config_vf_status(adapt, mmsch_idx_vf);
}

int amdgv_mmsch_clear_rb_decouple_status(struct amdgv_adapter *adapt,
						 uint32_t libgv_idx_vf)
{
	uint32_t mmsch_idx_vf;

	/* if not supported, return 0 */
	if (!adapt->mmsch.is_feature_enabled.cmd_buffer)
		return 0;

	if (AMDGV_IS_IDX_INVALID(libgv_idx_vf)) {
		AMDGV_ERROR("Invalid VF index\n");
		return AMDGV_FAILURE;
	}

	mmsch_idx_vf = CONVERT_TO_MMSCH_IDX_VF(libgv_idx_vf);

	/* try to turn on RB decouple */
	adapt->mmsch.rb_decouple.enabled_vfs |= 1 << mmsch_idx_vf;
	adapt->mmsch.rb_decouple.enabled_vfs &= adapt->mmsch.default_rb_decouple_enabled_vfs;

	return 0;
}

int amdgv_mmsch_read_all_output(struct amdgv_adapter *adapt, uint32_t idx_vf,
				enum amdgv_sched_block sched_block)
{
	/* By design, after we receive a MMSCH interrupt,
	 * we must read all MMSCH's output struct and compare timestamp.
	 * However, currently we only have BW config
	 */
	int ret = 0;

	uint32_t type;
	uint64_t timestamp_old;
	uint64_t timestamp_new;

	uint32_t vcn_engine_idx;
	uint32_t mmsch_idx_vf;
	uint32_t libgv_idx_vf;

	struct amdgv_mmsch_vcn_vf_status vf_status_backup[AMDGV_MMSCH_MAX_VCN_ENGINE][AMDGV_MAX_VF_SLOT];
	struct amdgv_mmsch_vcn_vf_status *vf_status_old;
	struct amdgv_mmsch_vcn_vf_status *vf_status_new;

	/* if not supported, return 0 */
	if (!adapt->mmsch.is_feature_enabled.cmd_buffer)
		return 0;

	if (amdgv_mmsch_sched_block_to_vcn_engine_idx(adapt, sched_block, &vcn_engine_idx))
		return AMDGV_FAILURE;

	for (type = AMDGV_MMSCH_CMD_TYPE_BANDWIDTH_CONFIG; type < AMDGV_MMSCH_CMD_TYPE_MAX; type++) {
		switch (type) {
		case AMDGV_MMSCH_CMD_TYPE_BANDWIDTH_CONFIG:
			/* back up original data */
			timestamp_old = (uint64_t)adapt->mmsch.bandwidth_config.output_gpu_timestamp_hi;
			timestamp_old <<= 32;
			timestamp_old |= (uint64_t)adapt->mmsch.bandwidth_config.output_gpu_timestamp_lo;
			oss_memcpy(&vf_status_backup,
					&adapt->mmsch.bandwidth_config.vcn_vf_status,
					sizeof(struct amdgv_mmsch_vcn_vf_status) * AMDGV_MMSCH_MAX_VCN_ENGINE * AMDGV_MAX_VF_SLOT);

			/* read output */
			if (amdgv_mmsch_read_bandwidth_config(adapt)) {
				AMDGV_ERROR("Can't read bandwidth config from framebuffer\n");
				ret = AMDGV_FAILURE;
				break;
			}

			timestamp_new = (uint64_t)adapt->mmsch.bandwidth_config.output_gpu_timestamp_hi;
			timestamp_new <<= 32;
			timestamp_new |= (uint64_t)adapt->mmsch.bandwidth_config.output_gpu_timestamp_lo;

			/* check timestamp */
			if (timestamp_new != timestamp_old) {
				/* check each vf_status */
				for (mmsch_idx_vf = 0; mmsch_idx_vf < adapt->num_vf + 1; mmsch_idx_vf++) {
					vf_status_old = &vf_status_backup[vcn_engine_idx][mmsch_idx_vf];
					vf_status_new = &adapt->mmsch.bandwidth_config.vcn_vf_status[vcn_engine_idx][mmsch_idx_vf];

					libgv_idx_vf = CONVERT_TO_LIBGV_IDX_VF(mmsch_idx_vf);

					/* the output bit from MMSCH and only change from 0 to 1 */
					if (vf_status_new->job_limit_not_supported != vf_status_old->job_limit_not_supported) {
						amdgv_put_error(libgv_idx_vf,
								AMDGV_ERROR_MMSCH_UNSUPPORTED_VCN_FW,
								vcn_engine_idx);
					}

					if (vf_status_new->job_ignored != vf_status_old->job_ignored) {
						amdgv_put_error(libgv_idx_vf,
								AMDGV_ERROR_MMSCH_IGNORED_JOB,
								vcn_engine_idx);
					}
				}
			}
			break;
		case AMDGV_MMSCH_CMD_TYPE_RB_DECOUPLE:
			// nothing to read
			break;
		default:
			AMDGV_ERROR("Should not reach here, MMSCH command type is invalid\n");
			ret = AMDGV_FAILURE;
			break;
		}
	}

	return ret;
}

int amdgv_mmsch_update_pf2vf_msg(struct amdgv_adapter *adapt, uint32_t libgv_idx_vf)
{
	uint32_t vcn_engine_idx;
	uint32_t mmsch_idx_vf;

	struct amdgv_mmsch_vcn_job_limit *job_limit;
	struct amd_sriov_msg_pf2vf_info *pf2vf_msg = adapt->pf2vf_msg;

	/* if not supported, return 0 */
	if (!adapt->mmsch.is_feature_enabled.cmd_buffer)
		return 0;

	pf2vf_msg->feature_flags.flags.mm_bw_management = 1;
	mmsch_idx_vf = CONVERT_TO_MMSCH_IDX_VF(libgv_idx_vf);

	if ((1 << mmsch_idx_vf) & adapt->mmsch.rb_decouple.enabled_vfs)
		pf2vf_msg->feature_flags.flags.vcn_rb_decouple = 1;
	else
		pf2vf_msg->feature_flags.flags.vcn_rb_decouple = 0;

	if ((1 << mmsch_idx_vf) & adapt->mmsch.rb_decouple.av1_disabled)
		pf2vf_msg->feature_flags.flags.av1_support = 0;
	else
		pf2vf_msg->feature_flags.flags.av1_support = 1;

	for (vcn_engine_idx = 0; vcn_engine_idx < adapt->mmsch.max_vcn_engine; vcn_engine_idx++) {
		if (adapt->mmsch.bandwidth_config.vcn_enabled_vfs[vcn_engine_idx] & (1 << mmsch_idx_vf)) {
			job_limit = &adapt->mmsch.bandwidth_config.vcn_vf_bandwidth[vcn_engine_idx][mmsch_idx_vf].job_limit;
			pf2vf_msg->mm_bw_management[vcn_engine_idx].decode_max_dimension_pixels = job_limit->decode_max_dimension_pixels;
			pf2vf_msg->mm_bw_management[vcn_engine_idx].decode_max_frame_pixels     = job_limit->decode_max_frame_pixels;
			pf2vf_msg->mm_bw_management[vcn_engine_idx].encode_max_dimension_pixels = job_limit->encode_max_dimension_pixels;
			pf2vf_msg->mm_bw_management[vcn_engine_idx].encode_max_frame_pixels     = job_limit->encode_max_frame_pixels;
		} else {
			pf2vf_msg->mm_bw_management[vcn_engine_idx].decode_max_dimension_pixels = 0;
			pf2vf_msg->mm_bw_management[vcn_engine_idx].decode_max_frame_pixels     = 0;
			pf2vf_msg->mm_bw_management[vcn_engine_idx].encode_max_dimension_pixels = 0;
			pf2vf_msg->mm_bw_management[vcn_engine_idx].encode_max_frame_pixels     = 0;
		}
	}

	return 0;
}

int amdgv_mmsch_dump_bandwidth_config(struct amdgv_adapter *adapt)
{
	uint32_t vcn_engine_idx;
	uint32_t idx_vf;
	uint64_t timestamp;

	struct amdgv_mmsch_bandwidth_config *bw_cfg;
	struct amdgv_mmsch_vcn_vf_bandwidth *vf_bw;
	struct amdgv_mmsch_vcn_vf_status *vf_status;

	/* if not supported, return 0 */
	if (!adapt->mmsch.is_feature_enabled.cmd_buffer)
		return 0;

	if (amdgv_mmsch_read_bandwidth_config(adapt))
		return AMDGV_FAILURE;

	bw_cfg = &adapt->mmsch.bandwidth_config;

	AMDGV_INFO("MMSCH flags = 0x%x\n", bw_cfg->flags);

	for (vcn_engine_idx = 0; vcn_engine_idx < adapt->mmsch.max_vcn_engine; vcn_engine_idx++) {
		AMDGV_INFO("VCN%u enabled_vfs = 0x%x (bit0 is PF)\n", vcn_engine_idx,
				bw_cfg->vcn_enabled_vfs[vcn_engine_idx]);

		vf_bw = &bw_cfg->vcn_vf_bandwidth[vcn_engine_idx][0];
		vf_status = &bw_cfg->vcn_vf_status[vcn_engine_idx][0];
		AMDGV_INFO("VCN%u PF\n", vcn_engine_idx);
		AMDGV_INFO("  decode_max_dim_pix = 0x%x\n",
				vf_bw->job_limit.decode_max_dimension_pixels);
		AMDGV_INFO("  decode_max_fm_pix  = 0x%x\n",
				vf_bw->job_limit.decode_max_frame_pixels);
		AMDGV_INFO("  encode_max_dim_pix = 0x%x\n",
				vf_bw->job_limit.encode_max_dimension_pixels);
		AMDGV_INFO("  encode_max_fm_pix  = 0x%x\n",
				vf_bw->job_limit.encode_max_frame_pixels);
		AMDGV_INFO("  partition_usecs    = 0x%x\n", vf_bw->partition_usecs);
		AMDGV_INFO("  status             = 0x%x\n", vf_status->allbits);

		for (idx_vf = 0; idx_vf < adapt->num_vf; idx_vf++) {
			vf_bw = &bw_cfg->vcn_vf_bandwidth[vcn_engine_idx][idx_vf + 1];
			vf_status = &bw_cfg->vcn_vf_status[vcn_engine_idx][idx_vf + 1];
			AMDGV_INFO("VCN%u VF%u\n", vcn_engine_idx, idx_vf);
			AMDGV_INFO("  decode_max_dim_pix = 0x%x\n",
					vf_bw->job_limit.decode_max_dimension_pixels);
			AMDGV_INFO("  decode_max_fm_pix  = 0x%x\n",
					vf_bw->job_limit.decode_max_frame_pixels);
			AMDGV_INFO("  encode_max_dim_pix = 0x%x\n",
					vf_bw->job_limit.encode_max_dimension_pixels);
			AMDGV_INFO("  encode_max_fm_pix  = 0x%x\n",
					vf_bw->job_limit.encode_max_frame_pixels);
			AMDGV_INFO("  partition_usecs    = 0x%x\n", vf_bw->partition_usecs);
			AMDGV_INFO("  status             = 0x%x\n", vf_status->allbits);
		}
	}

	timestamp = ((uint64_t)bw_cfg->output_gpu_timestamp_hi << 32) | (uint64_t)bw_cfg->output_gpu_timestamp_lo;
	AMDGV_INFO("MMSCH bw cmd timestamp = 0x%llx\n", timestamp);

	return 0;
}

/* All SRAM dump are surrounded by ifdef and should not be released */
int amdgv_mmsch_sram_dump(struct amdgv_adapter *adapt)
{
	int ret;

	uint32_t *ptr = (uint32_t *)amdgv_memmgr_get_cpu_addr(adapt->mmsch.sram_dump_mem);
	// uint32_t size = AMDGV_MMSCH_SRAM_DUMP_SIZE;

	/* if not supported, return 0 */
	if (!adapt->mmsch.is_feature_enabled.cmd_buffer)
		return 0;

	if (!ptr) {
		AMDGV_ERROR("SRAM DUMP output pointer is not allocated\n");
		return AMDGV_FAILURE;
	}

	/* index VF does not matter here */
	ret = amdgv_mmsch_submit_cmd(adapt, AMDGV_MMSCH_CMD_TYPE_SRAM_DUMP, AMDGV_PF_IDX);
	if (ret) {
		AMDGV_ERROR("Failed to perform MMSCH SRAM DUMP\n");
		return AMDGV_FAILURE;
	}

	/* wait a bit for MMSCH to dump sram */
	oss_msleep(200);

	/* write to binary file, if exist kernel_write function */
	return 0; // kernel_write(ptr, size);
}

int amdgv_mmsch_check_enabled_features(struct amdgv_adapter *adapt)
{
	adapt->mmsch.is_feature_enabled.allbits = 0;

	if (adapt->mmsch.mmsch_funcs)
		adapt->mmsch.mmsch_funcs->check_enabled_features(adapt);

	return 0;
}

int amdgv_mmsch_preconfig_vf(struct amdgv_adapter *adapt, uint32_t libgv_idx_vf)
{
	int ret = 0;

	ret |= amdgv_mmsch_clear_bandwidth_config_vf_status(adapt, libgv_idx_vf);
	ret |= amdgv_mmsch_clear_rb_decouple_status(adapt, libgv_idx_vf);

	return ret;
}

int amdgv_mmsch_config_vf(struct amdgv_adapter *adapt, uint32_t libgv_idx_vf)
{
	int ret;
	int cmd_ret;

	ret = amdgv_mmsch_preconfig_vf(adapt, libgv_idx_vf);
	if (!ret) {
		cmd_ret = amdgv_mmsch_submit_cmd(adapt, AMDGV_MMSCH_CMD_TYPE_BANDWIDTH_CONFIG, libgv_idx_vf);
		if (cmd_ret)
			AMDGV_WARN("failed to submit BW config. This will result limited BW management\n");
		ret = cmd_ret;

		cmd_ret = amdgv_mmsch_submit_cmd(adapt, AMDGV_MMSCH_CMD_TYPE_RB_DECOUPLE, libgv_idx_vf);
		if (cmd_ret)
			AMDGV_WARN("failed to enable RB decouple. This will result limited decode performance\n");
		ret |= cmd_ret;
	}

	return ret;
}

int amdgv_mmsch_reconfig_vf(struct amdgv_adapter *adapt, uint32_t libgv_idx_vf,
			struct amd_sriov_msg_vf2pf_info *vf2pf_msg)
{
	int ret = 0;
	uint32_t mmsch_idx_vf;

	mmsch_idx_vf = CONVERT_TO_MMSCH_IDX_VF(libgv_idx_vf);

	if (vf2pf_msg->os_info.info.is_mm_multiqueue == 1) {
		AMDGV_INFO("Guest OS has MM multiqueue\n");

		adapt->mmsch.rb_decouple.enabled_vfs &= ~(1 << mmsch_idx_vf);
		if (amdgv_mmsch_submit_cmd(adapt, AMDGV_MMSCH_CMD_TYPE_RB_DECOUPLE, libgv_idx_vf)) {
			AMDGV_ERROR("failed to disable RB decouple\n");
			ret = AMDGV_FAILURE;
		}
	}
	return ret;
}
