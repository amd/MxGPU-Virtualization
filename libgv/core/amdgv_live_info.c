/*
 * Copyright 2020-2023 Advanced Micro Devices, Inc.
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

#include "amdgv_live_info.h"
#include "amdgv_device.h"
#include "amdgv_vfmgr.h"
#include "amdgv_powerplay_swsmu.h"
#include "amdgv_ras.h"

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

enum amdgv_live_info_status amdgv_import_data_by_op(struct amdgv_adapter *adapt,
							   uint32_t data_op)
{
	enum amdgv_live_info_status status = AMDGV_LIVE_INFO_STATUS_FEATURE_NOT_SUPPORTED;
	void *gpu_data;
	struct live_info_table_header *header;
	uint32_t offset, op_num;

	if (!adapt->opt.skip_hw_init)
		return 0;

	gpu_data = adapt->sys_mem_info.va_ptr;
	offset = ((struct amdgv_gpu_data_v2 *)gpu_data)->header.op_offset[data_op];
	op_num = ((struct amdgv_gpu_data_v2 *)gpu_data)->header.op_num;

	header = (struct live_info_table_header *)((char *)gpu_data + offset);
	if (data_op < op_num) {
		amdgv_live_info_import_data(adapt, data_op, (void *)header,
						&status);
		if (status) {
			AMDGV_ERROR("Import %d data fail\n", data_op);
			adapt->opt.skip_hw_init = 0;
		}
	}
	return status;
}

enum amdgv_live_info_status amdgv_import_data(struct amdgv_adapter *adapt)
{
	enum amdgv_live_info_status status = AMDGV_LIVE_INFO_STATUS_FEATURE_NOT_SUPPORTED;
	void *gpu_data;
	struct live_info_table_header *header;
	uint32_t op_num, offset;
	uint32_t *op_offset;
	enum amdgv_live_info_data data_op;

	if (!adapt->opt.skip_hw_init)
		return 0;

	gpu_data  = adapt->sys_mem_info.va_ptr;
	op_num    =  ((struct amdgv_gpu_data_v2 *)gpu_data)->header.op_num;
	op_offset = &((struct amdgv_gpu_data_v2 *)gpu_data)->header.op_offset[0];

	for (data_op = AMDGV_LIVE_INFO_DATA__CRITICAL_STATE; data_op < op_num; data_op++) {
		if ((data_op != AMDGV_LIVE_INFO_DATA__MODULE_PARAM_PRE) &&
			(data_op != AMDGV_LIVE_INFO_DATA__MEMMGR) &&
			(data_op != AMDGV_LIVE_INFO_DATA__UNPROCESSED_EVENT)) {
			offset = op_offset[data_op];
			header = (struct live_info_table_header *)((char *)gpu_data + offset);
			amdgv_live_info_import_data(adapt, data_op, (void *)header,
							&status);
			if (status) {
				adapt->opt.skip_hw_init = 0;
				AMDGV_ERROR("Import %d data fail, Enable interrupt.\n", data_op);
				amdgv_toggle_interrupt(adapt, true);
				return status;
			}
		}
	}

	/* get ih info */
	if (!adapt->irqmgr.disable_parse_ih) {
		if (adapt->irqmgr.ih_funcs->get_rptr) {
			adapt->irqmgr.ih.rptr = adapt->irqmgr.ih_funcs->get_rptr(adapt);
			if (amdgv_ih_ring_set(adapt))
				status = AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
		} else {
			AMDGV_ERROR("Get_rptr function not implemented.\n");
			status = AMDGV_LIVE_INFO_STATUS_FEATURE_NOT_SUPPORTED;
		}
	}

	if (adapt->gfx.funcs && adapt->gfx.funcs->hw_init_internal_set) {
		adapt->gfx.funcs->hw_init_internal_set(adapt);
	}

	if (status) {
		adapt->opt.skip_hw_init = 0;
		AMDGV_ERROR("Get ih data fail, Enable interrupt.\n");
		amdgv_toggle_interrupt(adapt, true);
	}

	return status;
}

enum amdgv_live_info_status amdgv_export_data(struct amdgv_adapter *adapt)
{
	enum amdgv_live_info_status status = AMDGV_LIVE_INFO_STATUS_SUCCESS;
	void *gpu_data;
	struct live_info_table_header *header;
	uint32_t offset;
	enum amdgv_live_info_data data_op;

	if ((adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) || adapt->fini_opt.skip_hw_fini) {
		gpu_data = adapt->sys_mem_info.va_ptr;
		if (!gpu_data) {
			AMDGV_ERROR("no gpu_data!!\n");
			return 0;
		}
		for (data_op = AMDGV_LIVE_INFO_DATA__CRITICAL_STATE; data_op < AMDGV_LIVE_INFO_DATA__END; data_op++) {
			if (data_op != AMDGV_LIVE_INFO_DATA__IP_DISCOVERY) {
				offset = ((struct amdgv_gpu_data_v2 *)gpu_data)->header.op_offset[data_op];
				header = (struct live_info_table_header *)((char *)gpu_data + offset);
				amdgv_live_info_export_data(adapt, data_op, (void *)header, &status);
				if (status) {
					adapt->fini_opt.export_status = false;
					AMDGV_INFO("Export %d data fail\n", data_op);
					return status;
				}
			}
		}
	}

	if (adapt->fini_opt.skip_hw_fini)
		adapt->fini_opt.export_status = true;

	return status;
}

enum amdgv_live_info_status amdgv_live_info_init_metadata(struct amdgv_adapter *adapt)
{
	enum amdgv_live_info_status status = AMDGV_LIVE_INFO_STATUS_SUCCESS;
	struct amdgv_gpu_data_v2 *gpu_data = NULL;
	enum amdgv_live_info_data data_op;
	struct live_info_table_header *header;
	uint32_t offset = 0;
	uint32_t size_offset = 0;

	if ((adapt->flags & AMDGV_FLAG_GPUV_LIVE_UPDATE) || adapt->fini_opt.skip_hw_fini) {
		offset      = offsetof(struct amdgv_gpu_data_v2, crtical_state);
		size_offset = offsetof(struct amdgv_gpu_data_v2, header.size);
		gpu_data = (struct amdgv_gpu_data_v2 *)adapt->sys_mem_info.va_ptr;
		oss_memset(gpu_data, 0, AMDGV_LIVE_INFO_V2_SIZE);

		AMDGV_DEBUG("Live update initialize metadata start.\n");

		/* Get offsets for each function block's live data */
		for (data_op = AMDGV_LIVE_INFO_DATA__CRITICAL_STATE; data_op < AMDGV_LIVE_INFO_DATA__END; data_op++) {
			header = (struct live_info_table_header *)((char *)gpu_data + offset);
			gpu_data->header.op_offset[data_op] = offset;

			switch (data_op) {
			case AMDGV_LIVE_INFO_DATA__CRITICAL_STATE:
				header->structure_size = sizeof(struct amdgv_live_info_critical_state);
				break;
			case AMDGV_LIVE_INFO_DATA__VBIOS:
				header->structure_size = sizeof(struct amdgv_live_info_vbios);
				break;
			case AMDGV_LIVE_INFO_DATA__MEMMGR:
				header->structure_size = sizeof(struct amdgv_live_info_memmgr);
				break;
			case AMDGV_LIVE_INFO_DATA__GPUMON:
				header->structure_size = sizeof(struct amdgv_live_info_gpumon);
				break;
			case AMDGV_LIVE_INFO_DATA__MODULE_PARAM_PRE:
				header->structure_size = sizeof(struct amdgv_live_info_param);
				break;
			case AMDGV_LIVE_INFO_DATA__MODULE_PARAM_POST:
				// Already handled: Module Param Pre and Post share same live data struct
				break;
			case AMDGV_LIVE_INFO_DATA__ECC:
				header->structure_size = sizeof(struct amdgv_live_info_ecc);
				break;
			case AMDGV_LIVE_INFO_DATA__XGMI:
				header->structure_size = sizeof(struct amdgv_live_info_xgmi);
				break;
			case AMDGV_LIVE_INFO_DATA__FW_INFO:
				header->structure_size = sizeof(struct amdgv_live_info_fw_info);
				break;
			case AMDGV_LIVE_INFO_DATA__POWERPLAY:
				header->structure_size = sizeof(struct amdgv_live_info_powerplay);
				break;
			case AMDGV_LIVE_INFO_DATA__VF:
				header->structure_size = sizeof(struct amdgv_live_info_vf) * AMDGV_MAX_VF_LIVE;
				break;
			case AMDGV_LIVE_INFO_DATA__WORLD_SWITCH:
				header->structure_size = sizeof(struct amdgv_live_info_world_switch) * AMDGV_MAX_NUM_WORLD_SWITCH;
				break;
			case AMDGV_LIVE_INFO_DATA__WS_STATE:
				header->structure_size = sizeof(struct amdgv_live_info_ws_state) * AMDGV_MAX_NUM_HW_SCHED;
				break;
			case AMDGV_LIVE_INFO_DATA__SCHED:
				header->structure_size = sizeof(struct amdgv_live_info_sched);
				break;
			case AMDGV_LIVE_INFO_DATA__UNPROCESSED_EVENT:
				header->structure_size = sizeof(struct amdgv_live_info_unprocessed_event);
				break;
			case AMDGV_LIVE_INFO_DATA__PSP:
				header->structure_size = sizeof(struct amdgv_live_info_psp);
				break;
			case AMDGV_LIVE_INFO_DATA__FFBM:
				header->structure_size = sizeof(struct amdgv_live_info_ffbm);
				break;
			case AMDGV_LIVE_INFO_DATA__VF_EXTEND:
				header->structure_size = sizeof(struct amdgv_live_info_vf_extend) * AMDGV_MAX_VF_LIVE;
				break;
			case AMDGV_LIVE_INFO_DATA__RING:
				header->structure_size = sizeof(struct amdgv_live_info_ring);
				break;
			case AMDGV_LIVE_INFO_DATA__MCA:
				header->structure_size = sizeof(struct amdgv_live_info_mca);
				break;
			default:
				AMDGV_DEBUG("No live data struct for op %d in amdgv_live_info_data.\n", data_op);
				break;
			}

			AMDGV_DEBUG("op:%d, \t offset:%08x,\t size:%08x\t\n", data_op, offset, header->structure_size);
			offset = offset + header->structure_size;
		}

		gpu_data->header.size = offset - size_offset;
		gpu_data->header.op_num = AMDGV_LIVE_INFO_DATA__END;
		gpu_data->header.hash_addr = adapt->hash_addr;
		AMDGV_DEBUG("Live info size:%08x\n", gpu_data->header.size);
	}
	return status;
}

int amdgv_live_info_export_size(struct amdgv_adapter *adapt, uint32_t *size)
{
	*size = AMDGV_GPU_DATA_SIZE;
	return 0;
}

int amdgv_live_info_export_data(struct amdgv_adapter *adapt, uint32_t data_op,
					   void *data, enum amdgv_live_info_status *status)
{
	uint32_t ret = 0;
	if (adapt == NULL || data == NULL || status == NULL) {
		return AMDGV_FAILURE;
	}

	*status = AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;

	switch (data_op) {
	case AMDGV_LIVE_INFO_DATA__CRITICAL_STATE: {
		struct amdgv_live_info_critical_state *critical_state = data;

		critical_state->gpu_status = adapt->status;

		*status = AMDGV_LIVE_INFO_STATUS_SUCCESS;
		break;
	}
	case AMDGV_LIVE_INFO_DATA__MODULE_PARAM_PRE: {
		struct amdgv_live_info_param *param_info = data;

		param_info->customized_vf_config_mode = adapt->customized_vf_config_mode;
		param_info->num_vf = adapt->num_vf;
		param_info->fw_load_type = adapt->fw_load_type;
		param_info->debug_mode = adapt->debug.mode;
		param_info->log_level = adapt->log_level;
		param_info->log_mask = adapt->log_mask;
		param_info->flags = adapt->flags;

		param_info->allow_time_full_access = adapt->sched.allow_time_full_access;
		param_info->perf_mon_enable = adapt->opt.perf_mon_enable;

		param_info->accelerator_partition_mode = adapt->mcp.accelerator_partition_mode;
		param_info->memory_partition_mode = adapt->mcp.memory_partition_mode;

		param_info->partition_full_access_enable = adapt->opt.partition_full_access_enable;
		param_info->bad_page_record_threshold = adapt->opt.bad_page_record_threshold;

		param_info->max_cper_count = adapt->opt.max_cper_count;
		param_info->ras_vf_telemetry_policy = adapt->opt.ras_vf_telemetry_policy;

		*status = AMDGV_LIVE_INFO_STATUS_SUCCESS;
		break;
	}
	case AMDGV_LIVE_INFO_DATA__MODULE_PARAM_POST: {
		struct amdgv_live_info_param *param_info = data;
		struct amdgv_sched_world_switch *world_switch;

		if (amdgv_sched_get_world_switch(adapt, AMDGV_SCHED_SPATIAL_PARTITION_0,
						 AMDGV_SCHED_BLOCK_GFX, &world_switch)) {
			*status = AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
			break;
		}

		param_info->gfx_sched_mode = world_switch->sched_mode;

		*status = AMDGV_LIVE_INFO_STATUS_SUCCESS;
		break;
	}
	case AMDGV_LIVE_INFO_DATA__VBIOS: {
		*status = amdgv_vbios_export_live_data(adapt, (struct amdgv_live_info_vbios *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__FW_INFO: {
		*status = amdgv_psp_fw_info_export_live_data(adapt, (struct amdgv_live_info_fw_info *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__IP_DISCOVERY: {
		AMDGV_DEBUG("Skip export IP discovery data\n");
		*status = AMDGV_LIVE_INFO_STATUS_SUCCESS;
		break;
	}
	case AMDGV_LIVE_INFO_DATA__MEMMGR: {
		*status = amdgv_memmgr_export_live_data(adapt, (struct amdgv_live_info_memmgr *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__ECC: {
		*status = amdgv_ecc_export_live_data(adapt, (struct amdgv_live_info_ecc *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__GPUIOV: {
		AMDGV_DEBUG("Skip export GPUIOV data\n");
		*status = AMDGV_LIVE_INFO_STATUS_SUCCESS;
		break;
	}
	case AMDGV_LIVE_INFO_DATA__XGMI: {
		*status = amdgv_xgmi_export_live_data(adapt, (struct amdgv_live_info_xgmi *) data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__SCHED: {
		*status = amdgv_sched_export_live_data(adapt, (struct amdgv_live_info_sched *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__UNPROCESSED_EVENT: {
		*status = amdgv_sched_export_unprocessed_event(adapt, (struct amdgv_live_info_unprocessed_event *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__POWERPLAY: {
		struct smu_context *smu = NULL;
		struct amdgv_live_info_powerplay *powerplay = data;
		struct smu_table_context *table_context = NULL;

		smu = (struct smu_context *)(adapt->pp.smu_backend);
		table_context = (struct smu_table_context *)(smu->smu_table_context);

		powerplay->smu_features = smu->features;

		if (adapt->cg.gc)
			powerplay->clock_gating_features_flags = adapt->cg.gc->clock_gating_flags.u32All;

		// smu clk info
		powerplay->socclk = table_context->boot_values.socclk;
		powerplay->dcefclk = table_context->boot_values.dcefclk;
		*status = AMDGV_LIVE_INFO_STATUS_SUCCESS;

		break;
	}
	case AMDGV_LIVE_INFO_DATA__GPUMON: {
		*status = amdgv_gpumon_export_live_data((amdgv_dev_t)adapt, (struct amdgv_live_info_gpumon *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__VF: {
		*status = amdgv_vfmgr_export_live_data((amdgv_dev_t)adapt, (struct amdgv_live_info_vf *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__WORLD_SWITCH: {
		*status = amdgv_sched_ws_export_live_data(adapt, (struct amdgv_live_info_world_switch *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__WS_STATE: {
		*status = amdgv_ws_state_export_live_data(adapt, (struct amdgv_live_info_ws_state *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__PSP: {
		*status = amdgv_psp_export_live_data(adapt, (struct amdgv_live_info_psp *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__FFBM: {
		*status = AMDGV_LIVE_INFO_STATUS_SUCCESS;
		if (adapt->ffbm.enabled) {
			*status = amdgv_ffbm_export_spa(adapt, (struct amdgv_live_info_ffbm *)data);
		}
		break;
	}
	case AMDGV_LIVE_INFO_DATA__VF_EXTEND: {
		*status = amdgv_vfmgr_export_live_data_extend(adapt, (struct amdgv_live_info_vf_extend *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__RING: {
		*status = amdgv_ring_export_live_data(adapt, (struct amdgv_live_info_ring *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__MCA: {
		*status = amdgv_mca_export_live_data(adapt, (struct amdgv_live_info_mca *)data);
		break;
	}
	default:
		AMDGV_WARN("%d is an unknown data request\n", data_op);
		*status = AMDGV_LIVE_INFO_STATUS_OP_UNKNOWN;
		ret = AMDGV_FAILURE;
		break;
	}

	AMDGV_DEBUG("op: %d\n", data_op);
	return ret;
}

int amdgv_live_info_import_data(struct amdgv_adapter *adapt, uint32_t data_op,
					   void *data, enum amdgv_live_info_status *status)
{
	uint32_t ret = 0;

	if (adapt == NULL || (data_op != AMDGV_LIVE_INFO_DATA__IP_DISCOVERY && data == NULL) || status == NULL)
		return AMDGV_FAILURE;

	switch (data_op) {
	case AMDGV_LIVE_INFO_DATA__CRITICAL_STATE: {
		struct amdgv_live_info_critical_state *critical_state = data;

		/* STATUS_HW_INIT should be set after import_data is finished.
		 * adapt->status will be set after hw_init() called anyway so
		 * just skip it if driver was HW_INIT state when export_live_data.
		 */
		if (critical_state->gpu_status != AMDGV_STATUS_HW_INIT)
			adapt->status = critical_state->gpu_status;

		*status = AMDGV_LIVE_INFO_STATUS_SUCCESS;
		break;
	}
	case AMDGV_LIVE_INFO_DATA__MODULE_PARAM_PRE: {
		struct amdgv_live_info_param *param_info = data;

		adapt->customized_vf_config_mode = param_info->customized_vf_config_mode;
		adapt->num_vf = param_info->num_vf;
		adapt->fw_load_type = param_info->fw_load_type;
		amdgv_debug_set_mode(adapt, param_info->debug_mode);
		adapt->log_level = param_info->log_level;
		adapt->log_mask = param_info->log_mask;
		adapt->flags = param_info->flags;

		adapt->sched.allow_time_full_access = param_info->allow_time_full_access;
		adapt->opt.perf_mon_enable = param_info->perf_mon_enable;

		adapt->mcp.accelerator_partition_mode = param_info->accelerator_partition_mode;
		adapt->mcp.memory_partition_mode = param_info->memory_partition_mode;

		adapt->opt.partition_full_access_enable = param_info->partition_full_access_enable;
		if (adapt->opt.partition_full_access_enable)
			adapt->flags |= AMDGV_FLAG_ENABLE_PARTITION_FULL_ACCESS;
		else
			adapt->flags &= ~AMDGV_FLAG_ENABLE_PARTITION_FULL_ACCESS;
		adapt->opt.bad_page_record_threshold = param_info->bad_page_record_threshold;

		adapt->opt.max_cper_count = param_info->max_cper_count;
		adapt->opt.ras_vf_telemetry_policy = param_info->ras_vf_telemetry_policy;

		*status = AMDGV_LIVE_INFO_STATUS_SUCCESS;
		break;
	}
	case AMDGV_LIVE_INFO_DATA__MODULE_PARAM_POST: {
		struct amdgv_live_info_param *param_info = data;
		struct amdgv_sched_world_switch *world_switch;

		if (amdgv_sched_get_world_switch(adapt, AMDGV_SCHED_SPATIAL_PARTITION_0,
						 AMDGV_SCHED_BLOCK_GFX, &world_switch)) {
			*status = AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
			break;
		}

		world_switch->sched_mode = param_info->gfx_sched_mode;

		*status = AMDGV_LIVE_INFO_STATUS_SUCCESS;
		break;
	}
	case AMDGV_LIVE_INFO_DATA__VBIOS: {
		*status = amdgv_vbios_import_live_data(adapt, (struct amdgv_live_info_vbios *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__FW_INFO: {
		*status = amdgv_psp_fw_info_import_live_data(adapt, (struct amdgv_live_info_fw_info *)data);

		/* need to re-apply mmsch feature after import FW info */
		if (amdgv_mmsch_check_enabled_features(adapt) ||
			amdgv_mmsch_get_default_bandwidth_config(adapt, &adapt->mmsch.bandwidth_config))
			*status = AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;

		*status = AMDGV_LIVE_INFO_STATUS_SUCCESS;
		break;
	}
	case AMDGV_LIVE_INFO_DATA__IP_DISCOVERY: {
		if (adapt->ip_discovery.discover_ip) {
			if (adapt->ip_discovery.discover_ip(adapt)) {
				*status = AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
				return AMDGV_FAILURE;
			}
		} else {
			AMDGV_DEBUG("discover_ip function not implemented\n");
		}

		// Get vmhub info
		if (adapt->vbios.vmhub_hook)
			adapt->vbios.vmhub_hook(adapt);
		else
			AMDGV_DEBUG("vmhub_hook function not implemented\n");

		// Get MM capability
		if (adapt->vbios.get_mm_capability) {
			// Legacy
			if (adapt->vbios.get_mm_capability(adapt)) {
				*status = AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
				return AMDGV_FAILURE;
			}
		} else {
			// bandwidth config gets imported by amdgv_mmsch_get_default_bandwidth_config
			// after fw info import
			AMDGV_DEBUG("get_mm_capability legacy function not implemented\n");
		}

		*status = AMDGV_LIVE_INFO_STATUS_SUCCESS;
		break;
	}
	case AMDGV_LIVE_INFO_DATA__MEMMGR: {
		*status = amdgv_memmgr_import_live_data(adapt, (struct amdgv_live_info_memmgr *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__ECC: {
		if (adapt->live_info_funcs)
			*status = (adapt->live_info_funcs[AMDGV_LIVE_INFO_DATA__ECC]->import_live_data)(adapt, data);
		else
			*status = amdgv_ecc_import_live_data(adapt, (struct amdgv_live_info_ecc *)data);

		break;
	}
	case AMDGV_LIVE_INFO_DATA__GPUIOV: {
		*status = AMDGV_LIVE_INFO_STATUS_SUCCESS;
		if (adapt->gpuiov.funcs->get_config_info) {
			if (adapt->gpuiov.funcs->get_config_info(adapt)) {
				*status = AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
			}
		} else {
			AMDGV_ERROR("get_config_info function not implemented\n");
			*status = AMDGV_LIVE_INFO_STATUS_FEATURE_NOT_SUPPORTED;
		}
		break;
	}
	case AMDGV_LIVE_INFO_DATA__XGMI: {
		*status = amdgv_xgmi_import_live_data(adapt, (struct amdgv_live_info_xgmi *) data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__SCHED: {
		*status = amdgv_sched_import_live_data(adapt, (struct amdgv_live_info_sched *)data);
		break;
	}
	/* Unprocessed events should be imported after scheduler is resumed and running */
	case AMDGV_LIVE_INFO_DATA__UNPROCESSED_EVENT: {
		*status = amdgv_sched_import_unprocessed_event(adapt, (struct amdgv_live_info_unprocessed_event *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__POWERPLAY: {
		struct amdgv_live_info_powerplay *powerplay = data;
		struct smu_context *smu = NULL;
		struct smu_table_context *table_context = NULL;

		smu = (struct smu_context *)(adapt->pp.smu_backend);
		table_context = (struct smu_table_context *)(smu->smu_table_context);
		smu->features = powerplay->smu_features;

		if (adapt->cg.gc)
			adapt->cg.gc->clock_gating_flags.u32All = powerplay->clock_gating_features_flags;

		// parse smu table from vbios
		if (adapt->pp.pp_funcs->parse_smu_table_info) {
			if (adapt->pp.pp_funcs->parse_smu_table_info(adapt)) {
				*status = AMDGV_LIVE_INFO_STATUS_GENERIC_ERROR;
				return AMDGV_FAILURE;
			}
		} else {
			AMDGV_ERROR("parse_smu_table_info function not implemented\n");
			*status = AMDGV_LIVE_INFO_STATUS_FEATURE_NOT_SUPPORTED;
			return AMDGV_FAILURE;
		}
		// smu clk info
		table_context->boot_values.socclk = powerplay->socclk;
		table_context->boot_values.dcefclk = powerplay->dcefclk;
		*status = AMDGV_LIVE_INFO_STATUS_SUCCESS;
		break;
	}
	case AMDGV_LIVE_INFO_DATA__GPUMON: {
		*status = amdgv_gpumon_import_live_data(adapt, (struct amdgv_live_info_gpumon *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__VF: {
		*status = amdgv_vfmgr_import_live_data(adapt, (struct amdgv_live_info_vf *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__WORLD_SWITCH: {
		*status = amdgv_sched_ws_import_live_data(adapt, (struct amdgv_live_info_world_switch *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__WS_STATE: {
		*status = amdgv_ws_state_import_live_data(adapt, (struct amdgv_live_info_ws_state *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__PSP: {
		*status = amdgv_psp_import_live_data(adapt, (struct amdgv_live_info_psp *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__FFBM: {
		*status = AMDGV_LIVE_INFO_STATUS_SUCCESS;
		if (adapt->ffbm.enabled)
			*status = amdgv_ffbm_import_spa_and_gpa(adapt, (struct amdgv_live_info_ffbm *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__VF_EXTEND: {
		*status = amdgv_vfmgr_import_live_data_extend(adapt, (struct amdgv_live_info_vf_extend *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__RING: {
		*status = amdgv_ring_import_live_data(adapt, (struct amdgv_live_info_ring *)data);
		break;
	}
	case AMDGV_LIVE_INFO_DATA__MCA: {
		*status = amdgv_mca_import_live_data(adapt, (struct amdgv_live_info_mca *)data);
		break;
	}
	default:
		AMDGV_DEBUG("%d is an unknown data request\n", data_op);
		*status = AMDGV_LIVE_INFO_STATUS_OP_UNKNOWN;
		ret = AMDGV_FAILURE;
		break;
	}

	if (*status)
		ret = AMDGV_FAILURE;

	return ret;
}

static int amdgv_live_info_reset_gpu_timer_isr(void *context)
{
	uint32_t assigned_vf_count;
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)context;

	assigned_vf_count = oss_get_assigned_vf_count(adapt->dev, (adapt->xgmi.phy_nodes_num > 1));
	if (!assigned_vf_count) {
		amdgv_sched_queue_event(adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_FORCE_RESET_GPU, 0);
	}

	return 0;
}

/* When deferred_full_live_update is enabled, the driver
 * will try to trigger a WGR to load the new firmware.
 * To reduce the impact to minial, try to issue WGR when all
 * the VMs are shutdown or destroyed by checking the assgned
 * VF count.
 */
void amdgv_live_info_prepare_reset(struct amdgv_adapter *adapt)
{
	timer_t wgr_timer;
	uint32_t assigned_vf_count;

	if (!(adapt->opt.deferred_full_live_update && (adapt->flags & AMDGV_FLAG_MIDDLE_OF_LIVE_UPDATE)))
		return;

	/* If phy_node_num > 1, we need to make sure all the adapters' VFs
	 * are not assigned to any VM because the WGR will trigger a chain reset
	 * meanning that all the adapters will perform a WGR
	 */
	assigned_vf_count = oss_get_assigned_vf_count(adapt->dev, (adapt->xgmi.phy_nodes_num > 1));

	if (assigned_vf_count <= 1) {
		/* delay 500ms to check the count again because when
		 * host received REL_GPU_FINI or VF_FLR_INTRUPT from SMU,
		 * the vf's pci enable_cnt may still not decrease to 0.
		 */
		wgr_timer = oss_timer_init_ex(amdgv_live_info_reset_gpu_timer_isr,
										(void *)adapt, adapt->dev);
		if (oss_timer_start(wgr_timer, LIVE_INFO_DELAY_CHECK_USEC, OSS_TIMER_TYPE_ONE_TIME))
			AMDGV_ERROR("live update reset gpu timer start fail!\n");
	}
}

#ifdef __linux__
#define stringification(s)  _stringification(s)
#define _stringification(s) #s
_Static_assert(
	LIVE_INFO_MAX_GC_INSTANCES >= AMDGV_MAX_GC_INSTANCES,
	"LIVE_INFO_MAX_GC_INSTANCES must be >= AMDGV_MAX_GC_INSTANCES");

_Static_assert(
	LIVE_INFO_MAX_SDMA_RINGS >= AMDGV_MAX_SDMA_RINGS,
	"LIVE_INFO_MAX_SDMA_RINGS must be >= AMDGV_MAX_SDMA_RINGS");

_Static_assert(
	LIVE_INFO_MAX_COMPUTE_RINGS >= AMDGV_MAX_GC_INSTANCES * AMDGV_MAX_COMPUTE_RINGS,
	"LIVE_INFO_MAX_COMPUTE_RINGS must be >= AMDGV_MAX_GC_INSTANCES * AMDGV_MAX_COMPUTE_RINGS");
#undef _stringification
#undef stringification
#endif
