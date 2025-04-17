/*
 * Copyright (c) 2023 Advanced Micro Devices, Inc. All rights reserved.
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

#include <smi_drv.h>
#include <smi_drv_core.h>
#include <smi_drv_core_api.h>
#include <smi_drv_cmd.h>

extern struct gim_error_ring_buffer *gim_error_rb;

#define CHECK_SIZE(var, a, c) do { \
	if (sizeof(var->raw_ ## a) < sizeof(var->c)) { \
		gim_put_error(AMDGV_ERROR_DRIVER_INVALID_VALUE, \
			sizeof(var->c)); \
		kfree(var); \
		var = NULL; \
		return SMI_STATUS_INVAL; \
	} \
	} while (0)

int smi_cmd_handshake(struct smi_ctx *ctx, void *inb, void *outb,
		uint16_t in_len, uint16_t out_len)
{
	struct smi_handshake *hs_in;
	struct smi_handshake *hs_out;
	int ret = SMI_STATUS_SUCCESS;
	int cmd = 0;
	int max = 0;

	/* Check version */
	if ((in_len != sizeof(*hs_in)) || (out_len != sizeof(*hs_out)))
		return SMI_STATUS_INVAL;

	hs_in  = (struct smi_handshake *) inb;
	hs_out = (struct smi_handshake *) outb;

	/* Check if the input version is supported */
	switch (hs_in->version) {
	case SMI_VERSION_BETA_0:
	case SMI_VERSION_BETA_1:
	case SMI_VERSION_BETA_2:
	case SMI_VERSION_BETA_3:
	case SMI_VERSION_BETA_4:
		ctx->tbl_cmd = smi_oss_funcs->alloc_small_zero_memory(
			SMI_VERSION_BETA_1_NUM_CMD * sizeof(struct smi_cmd_entry));
		max = SMI_VERSION_BETA_1_NUM_CMD;

		if (!ctx->tbl_cmd)
			return SMI_STATUS_OUT_OF_RESOURCES;

		/* Assign functions */
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_VBIOS_INFO,
			smi_get_gpu_vbios_info,
			sizeof(struct smi_device_info),
			sizeof(struct smi_vbios_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_BOARD_INFO,
			smi_get_gpu_board_info,
			sizeof(struct smi_device_info),
			sizeof(struct smi_board_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_ASIC_INFO,
			smi_get_gpu_asic_info,
			sizeof(struct smi_device_info),
			sizeof(struct smi_asic_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_VRAM_INFO,
			smi_get_gpu_vram_info,
			sizeof(struct smi_device_info),
			sizeof(struct smi_vram_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_GPU_DRIVER_INFO,
			smi_get_gpu_driver_info,
			sizeof(struct smi_device_info),
			sizeof(struct smi_driver_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_POWER_CAP_INFO,
			smi_get_gpu_power_cap_info,
			sizeof(struct smi_device_info_ex),
			sizeof(struct smi_power_cap_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_PF_FB_INFO,
			smi_get_gpu_fb_info,
			sizeof(struct smi_device_info),
			sizeof(struct smi_pf_fb_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_GPU_CACHE_INFO,
			smi_get_gpu_cache_info,
			sizeof(struct smi_device_info),
			sizeof(struct smi_gpu_cache_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_SET_GPU_POWER_CAP,
			smi_set_gpu_power_cap,
			sizeof(struct smi_set_gpu_power_cap),
			0);
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_GPU_PERFORMANCE_INFO,
			smi_get_gpu_performance_info,
			sizeof(struct smi_device_info_ex),
			sizeof(struct smi_gpu_performance_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_VF_PARTITIONING_INFO,
			smi_get_vf_partition_info,
			sizeof(struct smi_device_info),
			sizeof(struct smi_vf_partition_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_SET_VF_PARTITIONING_INFO,
			smi_set_vf_partition_info,
			sizeof(struct smi_vf_partition_config),
			0);
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_CLEAR_VF_FB,
			smi_clear_vf_fb,
			sizeof(struct smi_device_info),
			0);
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_VF_STATIC_INFO,
			smi_get_vf_static_info,
			sizeof(struct smi_device_info),
			sizeof(struct smi_vf_static_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_VF_DYNAMIC_INFO,
			smi_get_vf_dynamic_info,
			sizeof(struct smi_device_info),
			sizeof(struct smi_vf_data));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_SERVER_STATIC_INFO,
			smi_get_server_static_info,
			0,
			sizeof(struct smi_server_static_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_CREATE_EVENT,
			smi_create_event_set,
			sizeof(struct smi_event_set_config),
			sizeof(smi_event_handle_t));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_ECC_STATUS,
			smi_get_ecc_info,
			sizeof(struct smi_device_info),
			sizeof(struct smi_ecc_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_BAD_PAGE_INFO,
			smi_query_bad_page_info,
			sizeof(struct smi_bad_page_info),
			0);
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_HANDLE,
			smi_get_handle_id,
			sizeof(struct smi_get_handle_info),
			sizeof(struct smi_get_handle_resp));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_GUEST_DATA,
			smi_get_guest_data,
			sizeof(struct smi_device_info),
			sizeof(struct smi_guest_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_DFC_FW_TABLE,
			smi_get_dfc_fw,
			sizeof(struct smi_device_info),
			sizeof(struct smi_dfc_fw));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_PCIE_INFO,
			smi_get_pcie_info,
			sizeof(struct smi_device_info),
			sizeof(struct smi_pcie_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_UCODE_ERR_RECORDS,
			smi_get_ucode_err_records,
			sizeof(struct smi_device_info),
			sizeof(struct smi_fw_error_record));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_VF_UCODE_INFO,
			smi_get_vf_ucode_info,
			sizeof(struct smi_device_info),
			sizeof(struct smi_fw_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_PARTITION_PROFILE_INFO,
			smi_get_partition_profile_info,
			sizeof(struct smi_device_info),
			sizeof(struct smi_profile_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_BLOCK_ECC_STATUS,
			smi_get_ecc_block_info,
			sizeof(struct smi_ras_query_if),
			sizeof(struct smi_ecc_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_LINK_METRICS,
			smi_get_link_metrics,
			sizeof(struct smi_device_info),
			sizeof(struct smi_link_metrics));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_LINK_TOPOLOGY,
			smi_get_link_topology,
			sizeof(struct smi_device_pair_info),
			sizeof(struct smi_link_topology));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_XGMI_FB_SHARING_CAPS,
			smi_get_xgmi_fb_sharing_caps,
			sizeof(struct smi_device_info),
			sizeof(union smi_xgmi_fb_sharing_caps));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_XGMI_FB_SHARING_MODE_INFO,
			smi_get_xgmi_fb_sharing_mode_info,
			sizeof(struct smi_xgmi_fb_sharing),
			sizeof(struct smi_xgmi_fb_sharing_flag));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_SMI_DATA,
			smi_get_data,
			sizeof(struct smi_data_query),
			sizeof(union smi_data));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_GPU_FW_INFO,
			smi_get_gpu_fw_info,
			sizeof(struct smi_device_info),
			sizeof(struct smi_fw_info));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_SET_XGMI_FB_SHARING_MODE,
			smi_set_xgmi_fb_sharing_mode,
			sizeof(struct smi_set_xgmi_fb_sharing_mode),
			0);
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_SET_XGMI_FB_SHARING_MODE_VER_2,
			smi_set_xgmi_fb_custom_sharing_mode,
			sizeof(struct smi_set_xgmi_fb_custom_sharing_mode),
			0);
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_READ_EVENT,
			smi_read_event_set,
			sizeof(struct smi_device_info),
			sizeof(struct smi_event_entry));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_DESTROY_EVENT,
			smi_destroy_event_set,
			sizeof(struct smi_device_info),
			0);
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_RAS_FEATURE_INFO,
			smi_get_ras_feature_info,
			sizeof(struct smi_device_info),
			sizeof(struct smi_ras_feature));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_METRICS_TABLE,
			smi_get_metrics_table,
			sizeof(struct smi_metrics_table),
			0);
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_ACCELERATOR_PARTITION_PROFILE_CONFIG,
			smi_get_accelerator_partition_profile_config,
			sizeof(struct smi_profile_configs),
			0);
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_GPU_ACCELERATOR_PARTITION,
			smi_get_accelerator_partition_profile,
			sizeof(struct smi_device_info),
			sizeof(struct smi_accelerator_partition_profile_cap));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_CURR_MEMORY_PARTITION_SETTING,
			smi_get_memory_partition_config,
			sizeof(struct smi_device_info),
			sizeof(struct smi_memory_partition_config));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_SET_GPU_ACCELERATOR_PARTITION_SETTING,
			smi_set_accelerator_partition_setting,
			sizeof(struct smi_set_gpu_accelerator_partition_setting),
			0);
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_SET_GPU_MEMORY_PARTITION_SETTING,
			smi_set_memory_partition_setting,
			sizeof(struct smi_set_gpu_memory_partition_setting),
			0);
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_SOC_PSTATE,
			smi_get_soc_pstate,
			sizeof(struct smi_device_info),
			sizeof(struct smi_dpm_policy));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_SET_SOC_PSTATE,
			smi_set_soc_pstate,
			sizeof(struct smi_set_dpm_policy),
			0);
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_GPU_DRIVER_MODEL,
			smi_get_gpu_driver_model,
			sizeof(struct smi_device_info),
			sizeof(struct smi_gpu_driver_model));
		SMI_ASSIGN_FUNC(ctx, cmd, SMI_CMD_CODE_GET_CPER,
			smi_get_cper_error,
			sizeof(struct smi_cper_config),
			0);

		/* Set max num of commands
		 * This needs to be set to the number of functions
		 * defined here
		 */
		ctx->max_cmd = cmd;
		/* Set the version */
		hs_out->version = hs_in->version;
		ctx->version = hs_out->version;
		break;

	default:
		/* The version is not supported */
		if (hs_in->version < SMI_VERSION_MIN)
			hs_out->version = SMI_VERSION_MIN;
		if (hs_in->version > SMI_VERSION_MAX)
			hs_out->version = SMI_VERSION_MAX;
		ret = SMI_STATUS_NOT_SUPPORTED;
		break;
	};
	return ret;
}

