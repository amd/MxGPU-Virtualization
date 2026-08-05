/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_sched_internal.h"
#include "amdgv_vfmgr.h"
#include "amdgv_guard.h"
#include "ras_sys.h"
#include "amdgv_ras_cmd.h"
#include "ta_ras_if.h"
#include "amdgv_asic.h"
#include "amdgv_gpumon.h"
#include "amdgv_psp_gfx_if.h"
#include "amdgv_ras_mgr.h"

#define AMDGPU_RAS_TYPE_RASCORE  0x1
#define AMDGPU_RAS_TYPE_BM       0x2
#define AMDGPU_RAS_TYPE_SRIOV    0x3

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

OSS_LIST_HEAD(g_ras_cmd_device_list);

int amdgv_ras_cmd_add_device(struct ras_core_context *ras_core)
{
	oss_list_add_tail(&ras_core->ras_cmd.head, &g_ras_cmd_device_list);
	return 0;
}

int amdgv_ras_cmd_remove_device(struct ras_core_context *ras_core)
{
	oss_list_del(&ras_core->ras_cmd.head);
	return 0;
}

static bool check_vf_ras_cmd(struct ras_core_context *ras_core, struct ras_cmd_param *param)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;

	if (!param)
		return false;

	if ((param->idx_vf < adapt->num_vf) && (param->idx_vf < AMDGV_MAX_VF_NUM))
		return true;

	return false;
}

static enum amdgv_ras_asic_type amd_asic_type_to_amdgv_cmd_asic_type(enum amd_asic_type asic_type)
{
	switch (asic_type) {
	case CHIP_MI300X:
		return AMDGV_RAS_CHIP_MI300X;
	case CHIP_MI308X:
		return AMDGV_RAS_CHIP_MI308X;
	case CHIP_LAST:
		return AMDGV_RAS_CHIP_LAST;
	default:
		return AMDGV_RAS_CHIP_UNKNOWN;
	}
}

static int amdgv_ras_query_interface_info(struct ras_core_context *ras_core,
	struct ras_cmd_ctx *cmd)
{
	struct ras_query_interface_info_rsp *ver_rsp =
	(struct ras_query_interface_info_rsp *)cmd->output_buff_raw;
	int ret;

	if (cmd->input_size != sizeof(struct ras_query_interface_info_req))
		return RAS_CMD__ERROR_INVALID_INPUT_SIZE;

	ret = ras_cmd_query_interface_info(ras_core, ver_rsp);
	if (!ret) {
		ver_rsp->plat_major_ver = 0;
		ver_rsp->plat_minor_ver = 0;
		ver_rsp->interface_type = RAS_CMD_INTERFACE_TYPE_PF;

		cmd->output_size = sizeof(struct ras_query_interface_info_rsp);
	}

	cmd->cmd_res = ret;

	return 0;
}

static struct ras_core_context *ras_cmd_get_ras_core(uint64_t dev_handle)
{
	struct ras_core_context *ras_core;

	if (!dev_handle || (dev_handle == RAS_CMD_DEV_HANDLE_MAGIC))
		return NULL;

	ras_core = (struct ras_core_context *)(dev_handle ^ RAS_CMD_DEV_HANDLE_MAGIC);

	if (ras_cmd_get_dev_handle(ras_core) == dev_handle)
		return ras_core;

	return NULL;
}

static int ras_get_device_info(struct ras_core_context *ras_core, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct ras_cmd_dev_info *dev_info = (struct ras_cmd_dev_info *)data;
	struct ras_cmd_get_ras_cap_req req = {0};
	struct ras_cmd_get_ras_cap_rsp rsp = {0};
	union amdgv_dev_info dev_data;
	uint32_t asic_type;

	if (!amdgv_gpumon_get_asic_type(adapt, &asic_type))
		dev_info->asic_type = amd_asic_type_to_amdgv_cmd_asic_type(asic_type);
	else
		dev_info->asic_type = AMDGV_RAS_CHIP_UNKNOWN;

	if (!amdgv_get_dev_info(adapt, AMDGV_GET_ENABLED_VF_NUM, &dev_data))
		dev_info->vf_num = dev_data.vf.num_enabled_vf;
	else
		dev_info->vf_num =  0;

	if (amdgv_ras_mgr_handle_ras_cmd(adapt, RAS_CMD__GET_RAS_CAP,
			&req, sizeof(struct ras_cmd_get_ras_cap_req),
			&rsp, sizeof(struct ras_cmd_get_ras_cap_rsp))) {
		dev_info->ecc_type = 0;
		dev_info->block_mask_bits = 0;
	} else {
		dev_info->ecc_type = rsp.ext_ecc_type;
		dev_info->block_mask_bits = rsp.ras_block_mask;
	}

	if (!amdgv_get_dev_info(adapt, AMDGV_GET_OAM_IDX, &dev_data))
		dev_info->oam_id = dev_data.oam.oam_idx;

	dev_info->location_id = adapt->bdf;

	return 0;
}

static int amdgv_ras_get_devices_info(struct ras_core_context *ras_core,
	struct ras_cmd_ctx *cmd)
{
	struct ras_cmd_devices_info_rsp *output_data =
			(struct ras_cmd_devices_info_rsp *)cmd->output_buff_raw;
	struct ras_cmd_mgr *cmd_mgr;
	struct ras_cmd_dev_info *dev_info;
	int idx = 0, ret;

	oss_list_for_each_entry(cmd_mgr, &g_ras_cmd_device_list, struct ras_cmd_mgr, head) {
		dev_info = &output_data->devs[idx];
		ret = ras_get_device_info(cmd_mgr->ras_core, dev_info);
		if (ret)
			return ret;

		dev_info->dev_handle = ras_cmd_get_dev_handle(cmd_mgr->ras_core);
		idx++;
	}

	output_data->dev_num = idx;
	output_data->version = 1;
	cmd->output_size = sizeof(struct ras_cmd_devices_info_rsp);

	return 0;
}

static int amdgv_ras_trigger_error_prepare(struct ras_core_context *ras_core,
			struct ras_cmd_inject_error_req *block_info)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;

	if (block_info->block_id == TA_RAS_BLOCK__XGMI_WAFL) {
		if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->set_df_cstate)
			if (adapt->pp.pp_funcs->set_df_cstate(adapt, DF_CSTATE_DISALLOW))
				AMDGV_WARN("Failed to disallow df cstate");

		if (amdgv_gpumon_set_pm_policy_level(adapt,
			AMDGV_PP_PM_POLICY_XGMI_PLPD,
			PP_XGMI_PLPD_MODE_DISABLE))
			AMDGV_WARN("Failed to disallow XGMI power down");
	}

	return 0;
}

static int amdgv_ras_trigger_error_end(struct ras_core_context *ras_core,
		struct ras_cmd_inject_error_req *block_info)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;

	if (block_info->block_id == TA_RAS_BLOCK__XGMI_WAFL) {
		if (amdgv_ras_intr_triggered())
			return 0;

		if (amdgv_gpumon_set_pm_policy_level(adapt,
			AMDGV_PP_PM_POLICY_XGMI_PLPD,
			PP_XGMI_PLPD_MODE_ENABLE))
			AMDGV_WARN("Failed to enable XGMI power down");

		if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->set_df_cstate)
			if (adapt->pp.pp_funcs->set_df_cstate(adapt, DF_CSTATE_ALLOW))
				AMDGV_WARN("Failed to allow df cstate");
	}

	return 0;
}

static uint64_t amdgv_ras_gpa_to_spa(struct amdgv_adapter *adapt, uint64_t gpa, uint32_t vf_idx)
{
	uint64_t addr = 0;

	if (vf_idx < adapt->num_vf) {
		/* VF gpa convert to spa address */
		addr = 	MBYTES_TO_BYTES(adapt->array_vf[vf_idx].fb_offset) + gpa;
		AMDGV_INFO("vf_idx:%d, fb_offset:0x%x, gpa:0x%llx\n",
			vf_idx, MBYTES_TO_BYTES(adapt->array_vf[vf_idx].fb_offset), gpa);
	} else {
		/* PF gpa convert to spa address */
		addr = gpa;
	}

	return addr;
}

static uint64_t amdgv_ras_spa_to_xgmi_global_addr(struct amdgv_adapter *adapt, uint64_t spa)
{
	AMDGV_INFO("phy_node_id:%d, node_segment_size:0x%llx, spa:0x%llx\n",
		adapt->xgmi.phy_node_id, adapt->xgmi.node_segment_size, spa);

	return (adapt->xgmi.phy_node_id * adapt->xgmi.node_segment_size + spa);
}

static int amdgv_ras_inject_error(struct ras_core_context *ras_core,
			struct ras_cmd_ctx *cmd, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct ras_cmd_inject_error_req *req =
		(struct ras_cmd_inject_error_req *)cmd->input_buff_raw;
	uint64_t spa;
	int ret = 0;

	if (req->block_id == RAS_BLOCK_ID__UMC) {
		spa = amdgv_ras_gpa_to_spa(adapt, req->address, req->vf_idx);
		if (amdgv_ras_mgr_check_retired_addr(adapt, spa)) {
			if (req->vf_idx < adapt->num_vf)
				RAS_DEV_WARN(ras_core->dev,
					"RAS WARN: inject: 0x%llx of VF %u has "
					"already been marked as bad!\n",
					req->address, req->vf_idx);
			else
				RAS_DEV_WARN(ras_core->dev, "RAS WARN: inject: 0x%llx has "
					"already been marked as bad!\n", spa);

			return RAS_CMD__ERROR_ACCESS_DENIED;
		}

		if (spa >= MBYTES_TO_BYTES(adapt->gpuiov.total_fb_avail)) {
			if (req->vf_idx < adapt->num_vf)
				RAS_DEV_WARN(ras_core->dev,
					"Invalid inject address 0x%llx of VF %u "
					"exceeds the umc address range!\n",
					req->address, req->vf_idx);
			else
				RAS_DEV_WARN(ras_core->dev, "Invalid inject address 0x%llx "
					"exceeds the umc address range!\n", spa);

			return RAS_CMD__ERROR_ACCESS_DENIED;
		}

		req->address = amdgv_ras_spa_to_xgmi_global_addr(adapt, spa);
	}

	amdgv_ras_trigger_error_prepare(ras_core, req);
	ret = rascore_handle_cmd(ras_core, cmd, data);
	amdgv_ras_trigger_error_end(ras_core, req);

	return ret;
}

static int amdgv_ras_get_block_ecc_info(struct ras_core_context *ras_core,
				struct ras_cmd_ctx *cmd, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct ras_cmd_block_ecc_info_req *input_data =
		(struct ras_cmd_block_ecc_info_req *)cmd->input_buff_raw;
	struct ras_cmd_block_ecc_info_rsp *output_data =
		(struct ras_cmd_block_ecc_info_rsp *)cmd->output_buff_raw;
	struct ras_cmd_param *vf_cmd = (struct ras_cmd_param *)data;
	struct vf_ras_lifespan_data *vf_ras;
	struct ras_cmd_block_ecc *baseline;
	int ret;

	if (input_data->block_id >= RAS_BLOCK_ID__LAST)
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	/* Check VF ras block permissions */
	if (vf_cmd &&
	    !(ras_core_get_ras_caps(ras_core) & BIT_ULL(input_data->block_id)))
		return RAS_CMD__ERROR_ACCESS_DENIED;

	if (!ras_mgr)
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	ret = rascore_handle_cmd(ras_core, cmd, data);
	if (!ret && vf_cmd) {
		vf_ras = &ras_mgr->array_vf[vf_cmd->idx_vf];
		baseline = &vf_ras->ecc_count_baseline[input_data->block_id];

		output_data->ce_count = output_data->ce_count > baseline->ce_count ?
				output_data->ce_count - baseline->ce_count : 0;
		output_data->ue_count = output_data->ue_count > baseline->ue_count ?
				output_data->ue_count - baseline->ue_count : 0;
		output_data->de_count = output_data->de_count > baseline->de_count ?
				output_data->de_count - baseline->de_count : 0;
	}

	return ret;
}

static int amdgv_ras_load_ta(struct ras_core_context *ras_core,
			struct ras_cmd_ctx *cmd, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct ras_cmd_ras_ta_load_req *input_data =
			(struct ras_cmd_ras_ta_load_req *)cmd->input_buff_raw;
	struct ras_cmd_ras_ta_load_rsp *out_data =
			(struct ras_cmd_ras_ta_load_rsp *)cmd->output_buff_raw;
	struct ras_psp_ta_load ras_ta_load = {0};
	amdgv_dev_t adev = ras_core->dev;
	uint32_t ta_version = 0, loaded_version = 0;
	enum ras_ta_load_status ta_load_status;
	uint8_t *buf_ptr;

	if (cmd->input_size != sizeof(*input_data))
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	if (!input_data->version || !input_data->data_len || !input_data->data_addr) {
		RAS_DEV_ERR(adev, "Invalid ras ta parameter: version:0x%x,"
			" data_len:0x%x, data_addr:0x%llx\n",
			input_data->version, input_data->data_len, input_data->data_addr);
		return RAS_CMD__ERROR_GENERIC;
	}

	buf_ptr = (uint8_t *)oss_zalloc(input_data->data_len);
	if (!buf_ptr) {
		RAS_DEV_ERR(adev, "Failed to alloc memory!\n");
		return RAS_CMD__ERROR_GENERIC;
	}

	if (oss_copy_from_user(buf_ptr, (uint8_t *)input_data->data_addr, input_data->data_len)) {
		RAS_DEV_ERR(adev, "Failed to copy data from user!\n");
		goto load_err;
	}

	if (amdgv_gpumon_ras_get_ta_version(adev, buf_ptr, input_data->data_len, &ta_version)) {
		RAS_DEV_ERR(adev, "Failed to get ras ta version!\n");
		goto load_err;
	}

	if (amdgv_gpumon_ras_get_loaded_ta_version(adev, &loaded_version))
		ta_load_status = RAS_TA_STATUS_LOADED;
	else  if (ta_version == loaded_version)
		ta_load_status = RAS_TA_STATUS_NO_CHANGE;
	else if (ta_version > loaded_version)
		ta_load_status = RAS_TA_STATUS_UPGRADED;
	else if (ta_version < loaded_version)
		ta_load_status = RAS_TA_STATUS_DOWNGRADED;
	else
		goto load_err;

	if (ta_load_status != RAS_TA_STATUS_NO_CHANGE) {
		ras_ta_load.fw_version = input_data->version;
		ras_ta_load.bin_size = input_data->data_len;
		ras_ta_load.bin_addr = buf_ptr;
		if (ras_psp_sideload_ras_ta(ras_core, &ras_ta_load)) {
			RAS_DEV_ERR(adev, "Failed to load RAS TA!\n");
			goto load_err;
		}
		/* Staging copy: sideload memcpy'd into PSP FW mem; drop user buffer. */
		oss_free(buf_ptr);
		buf_ptr = NULL;
		out_data->ras_session_id = ras_ta_load.out_session_id;
		adapt->psp.ras_context.ras_initialized = true;
		adapt->psp.fw_info[AMDGV_FIRMWARE_ID__RAS_TA] = ras_ta_load.out_loaded_ta_version;
	} else {
		oss_free(buf_ptr);
		amdgv_gpumon_get_ras_session_id(adev, &(out_data->ras_session_id));
	}

	out_data->ta_status = ta_load_status;

	cmd->output_size = sizeof(struct ras_cmd_ras_ta_load_rsp);

	return RAS_CMD__SUCCESS;

load_err:
	oss_free(buf_ptr);
	return RAS_CMD__ERROR_GENERIC;
}

static int amdgv_ras_unload_ta(struct ras_core_context *ras_core,
			struct ras_cmd_ctx *cmd, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct ras_cmd_ras_ta_unload_req *input_data =
			(struct ras_cmd_ras_ta_unload_req *)cmd->input_buff_raw;
	struct ras_psp_ta_unload ras_ta_unload = {0};

	if (cmd->input_size != sizeof(*input_data))
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	ras_ta_unload.ras_session_id = input_data->ras_session_id;

	if (ras_psp_unsideload_ras_ta(ras_core, &ras_ta_unload))
		return RAS_CMD__ERROR_GENERIC;

	adapt->psp.ras_context.ras_initialized = false;

	return RAS_CMD__SUCCESS;
}

static int amdgv_ras_get_vf_safe_range(struct amdgv_adapter *adapt,
			uint64_t *offset, uint64_t *size, uint32_t idx_vf)
{
	struct amdgv_vf_device *entry;
	uint64_t reserved_start;

	if (idx_vf == AMDGV_PF_IDX) {
		if (amdgv_vfmgr_configured_vf_num(adapt)) {
			*offset = 0;
			*size = 0;
		} else {
			/* All usable FB outside of PF region is considered safe */
			entry = &adapt->array_vf[idx_vf];
			*offset = MBYTES_TO_BYTES(entry->fb_size);
			*size = MBYTES_TO_BYTES(adapt->gpuiov.total_fb_usable) -
				MBYTES_TO_BYTES(entry->fb_size);
		}
	} else {
		entry = &adapt->array_vf[idx_vf];
		if (entry->configured) {
			reserved_start = GET_VF_TABLE_OFFSET_BY_ID(adapt, idx_vf, DATAEXCHANGE) +
				KBYTES_TO_BYTES(GET_VF_TABLE_SIZE_KB_BY_ID(adapt, idx_vf, DATAEXCHANGE));
			/* Add an additional 4MB to critical range
			* to account for guest sw init reservations
			*
			* TODO: review how guest can take this into account
			* and add this change later.
			* reserved_start += KBYTES_TO_BYTES(0x1000);
			*/
			*offset = reserved_start + MBYTES_TO_BYTES(entry->fb_offset);
			*size = MBYTES_TO_BYTES(entry->real_fb_size) -
				reserved_start - AMDGV_IP_DISCOVERY_OFFSET;
		} else {
			*offset = 0;
			*size = 0;
		}
	}

	return 0;
}

static int amdgv_ras_get_ras_safe_fb_addr_ranges(struct ras_core_context *ras_core,
	struct ras_cmd_ctx *cmd, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct ras_cmd_dev_handle *input_data =
			(struct ras_cmd_dev_handle *)cmd->input_buff_raw;
	struct ras_cmd_ras_safe_fb_address_ranges_rsp *ranges =
			(struct ras_cmd_ras_safe_fb_address_ranges_rsp *)cmd->output_buff_raw;
	uint32_t i = 0;

	if (cmd->input_size != sizeof(*input_data))
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	ranges->num_ranges = 0;

	/* VFs */
	for (i = 0; i < adapt->num_vf; i++) {
		if (ranges->num_ranges >= AMDGV_GPUMON_RAS_MAX_NUM_SAFE_RANGES)
			return RAS_CMD__SUCCESS_EXEED_BUFFER;

		if (amdgv_ras_get_vf_safe_range(adapt,
				&(ranges->range[ranges->num_ranges].start),
				&(ranges->range[ranges->num_ranges].size), i))
			return RAS_CMD__ERROR_GENERIC;

		if (ranges->range[ranges->num_ranges].size) {
			ranges->range[ranges->num_ranges].idx = i;
			ranges->num_ranges++;
		}
	}

	/* PF */
	if (ranges->num_ranges >= AMDGV_GPUMON_RAS_MAX_NUM_SAFE_RANGES)
		return RAS_CMD__SUCCESS_EXEED_BUFFER;

	if (amdgv_ras_get_vf_safe_range(adapt,
			&(ranges->range[ranges->num_ranges].start),
			&(ranges->range[ranges->num_ranges].size), AMDGV_PF_IDX))
		return RAS_CMD__ERROR_GENERIC;

	if (ranges->range[ranges->num_ranges].size) {
		ranges->range[ranges->num_ranges].idx = AMDGV_PF_IDX;
		ranges->num_ranges++;
	}

	ranges->version = 0;
	cmd->output_size = sizeof(struct ras_cmd_ras_safe_fb_address_ranges_rsp);

	return 0;
}

static int ras_translate_fb_address(struct ras_core_context *ras_core,
		enum ras_fb_addr_type src_type,
		enum ras_fb_addr_type dest_type,
		union ras_translate_fb_address *src_addr,
		union ras_translate_fb_address *dest_addr)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	uint64_t soc_phy_addr;
	int ret = 0;

	/* Does not need to be queued as event as this is a SW translation */
	switch (src_type) {
	case RAS_FB_ADDR_SOC_PHY:
		soc_phy_addr = src_addr->soc_phy_addr;
		break;
	case RAS_FB_ADDR_BANK:
		ret = ras_cmd_translate_bank_to_soc_pa(ras_core,
					src_addr->bank_addr, &soc_phy_addr);
		if (ret)
			return RAS_CMD__ERROR_GENERIC;
		break;
	case RAS_FB_ADDR_VF_PHY:
		if (src_addr->vf_phy_addr.vf_idx == AMDGV_PF_IDX ||
			!adapt->array_vf[src_addr->vf_phy_addr.vf_idx].configured)
			return RAS_CMD__ERROR_GENERIC;
		ret = amdgv_umc_local_gpa_to_spa(ras_core->dev,
			src_addr->vf_phy_addr.addr, src_addr->vf_phy_addr.vf_idx,
			&soc_phy_addr);
		if (ret)
			return RAS_CMD__ERROR_GENERIC;
		break;
	default:
		return RAS_CMD__ERROR_INVALID_CMD;
	}

	switch (dest_type) {
	case RAS_FB_ADDR_SOC_PHY:
		dest_addr->soc_phy_addr = soc_phy_addr;
		break;
	case RAS_FB_ADDR_BANK:
		ret = ras_cmd_translate_soc_pa_to_bank(ras_core,
				soc_phy_addr, &dest_addr->bank_addr);
		if (ret)
			return RAS_CMD__ERROR_GENERIC;
		break;
	case RAS_FB_ADDR_VF_PHY:
		ret = amdgv_umc_local_spa_to_gpa(ras_core->dev, soc_phy_addr,
			&dest_addr->vf_phy_addr.addr, &dest_addr->vf_phy_addr.vf_idx);
		if (ret)
			return RAS_CMD__ERROR_GENERIC;
		break;
	default:
		return RAS_CMD__ERROR_INVALID_CMD;
	}

	return ret;
}

static int amdgv_ras_translate_fb_address(struct ras_core_context *ras_core,
				struct ras_cmd_ctx *cmd, void *data)
{
	struct ras_cmd_translate_fb_address_req *req_buff =
			(struct ras_cmd_translate_fb_address_req *)cmd->input_buff_raw;
	struct ras_cmd_translate_fb_address_rsp *rsp_buff =
			(struct ras_cmd_translate_fb_address_rsp *)cmd->output_buff_raw;
	int ret = RAS_CMD__ERROR_GENERIC;

	if (cmd->input_size != sizeof(struct ras_cmd_translate_fb_address_req))
		return RAS_CMD__ERROR_INVALID_INPUT_SIZE;

	if ((req_buff->src_addr_type >= RAS_FB_ADDR_UNKNOWN) ||
	    (req_buff->dest_addr_type >= RAS_FB_ADDR_UNKNOWN))
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	ret = ras_translate_fb_address(ras_core, req_buff->src_addr_type,
			req_buff->dest_addr_type, &req_buff->trans_addr, &rsp_buff->trans_addr);
	if (ret)
		return RAS_CMD__ERROR_GENERIC;

	rsp_buff->version = 0;
	cmd->output_size = sizeof(struct ras_cmd_translate_fb_address_rsp);

	return RAS_CMD__SUCCESS;
}

static int amdgv_ras_get_link_topology(struct ras_core_context *ras_core,
				struct ras_cmd_ctx *cmd, void *data)
{
	struct ras_dev_link_topology_req *input_data =
			(struct ras_dev_link_topology_req *) cmd->input_buff_raw;
	struct ras_dev_link_topology_rsp *output_data =
			(struct ras_dev_link_topology_rsp *)cmd->output_buff_raw;
	struct amdgv_gpumon_link_topology_info link_topology;
	struct ras_core_context *src_ras_core, *dst_ras_core;
	int ret;

	if (cmd->input_size != sizeof(struct ras_dev_link_topology_req))
		return RAS_CMD__ERROR_INVALID_INPUT_SIZE;

	src_ras_core = ras_cmd_get_ras_core(input_data->src.dev_handle);
	if (!src_ras_core)
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	dst_ras_core = ras_cmd_get_ras_core(input_data->dst.dev_handle);
	if (!dst_ras_core)
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	ret = amdgv_gpumon_get_link_topology(src_ras_core->dev, dst_ras_core->dev, &link_topology);
	if (ret)
		return RAS_CMD__ERROR_GENERIC;

	output_data->link_status = link_topology.link_status;
	output_data->link_type = link_topology.link_type;
	output_data->num_hops = link_topology.num_hops;

	output_data->version = 0;
	cmd->output_size = sizeof(struct ras_dev_link_topology_rsp);
	return RAS_CMD__SUCCESS;
}

static int amdgv_get_pf_regions(struct ras_core_context *ras_core,
			struct ras_cmd_fb_regions_rsp *output_data)
{
	struct amdgv_fb_regions fb_regions;

	if (amdgv_get_fb_regions_info(ras_core->dev, 0, &fb_regions))
		return RAS_CMD__ERROR_GENERIC;

	output_data->regions[0].type = AMDGV_REGION_PF_DATA_EXCHANGE;
	output_data->regions[0].start = fb_regions.pf_dataexchange.offset;
	output_data->regions[0].size = fb_regions.pf_dataexchange.size;

	output_data->regions[1].type = AMDGV_REGION_CSA;
	output_data->regions[1].start = fb_regions.csa.offset;
	output_data->regions[1].size = fb_regions.csa.size;

	output_data->regions[2].type = AMDGV_REGION_TMR;
	output_data->regions[2].start = fb_regions.tmr.offset;
	output_data->regions[2].size = fb_regions.tmr.size;

	output_data->regions[3].type = AMDGV_REGION_PF_IP_DISCOVERY;
	output_data->regions[3].start = fb_regions.pf_ipd.offset;
	output_data->regions[3].size = fb_regions.pf_ipd.size;

	output_data->reg_cnt = 4;

	return RAS_CMD__SUCCESS;
}

static int amdgv_get_vf_regions(struct ras_core_context *ras_core,
			uint32_t vf_idx, struct ras_cmd_fb_regions_rsp *output_data)
{
	struct amdgv_fb_regions fb_regions;
	union amdgv_dev_info dev_info = {0};
	union amdgv_vf_info *vf_info;

	if (amdgv_get_dev_info(ras_core->dev, AMDGV_GET_ENABLED_VF_NUM, &dev_info))
		return RAS_CMD__ERROR_GENERIC;

	if (vf_idx >= dev_info.vf.num_enabled_vf)
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	vf_info = oss_zalloc(sizeof(*vf_info));
	if (!vf_info)
		return RAS_CMD__ERROR_GENERIC;

	if (amdgv_get_vf_info(ras_core->dev, vf_idx, AMDGV_GET_VF_FB, vf_info)) {
		oss_free(vf_info);
		return RAS_CMD__ERROR_GENERIC;
	}

	output_data->regions[0].type = AMDGV_REGION_VF_FB;
	output_data->regions[0].start = MBYTES_TO_BYTES(vf_info->fb.fb_offset);
	output_data->regions[0].size = MBYTES_TO_BYTES(vf_info->fb.fb_size);
	oss_free(vf_info);

	if (amdgv_get_fb_regions_info(ras_core->dev, vf_idx, &fb_regions))
		return RAS_CMD__ERROR_GENERIC;

	output_data->regions[1].type = AMDGV_REGION_VF_DATA_EXCHANGE;
	output_data->regions[1].start = fb_regions.vf_dataexchange.offset;
	output_data->regions[1].size = fb_regions.vf_dataexchange.size;

	output_data->regions[2].type = AMDGV_REGION_VF_IP_DISCOVERY;
	output_data->regions[2].start = fb_regions.vf_ipd.offset;
	output_data->regions[2].size = fb_regions.vf_ipd.size;

	output_data->reg_cnt = 3;

	return RAS_CMD__SUCCESS;
}

static int amdgv_ras_fb_regions(struct ras_core_context *ras_core,
		struct ras_cmd_ctx *cmd, void *data)
{
	struct ras_cmd_fb_region_req *input_data =
			(struct ras_cmd_fb_region_req *) cmd->input_buff_raw;
	struct ras_cmd_fb_regions_rsp *output_data =
			(struct ras_cmd_fb_regions_rsp *)cmd->output_buff_raw;
	int ret;

	if (input_data->vf_idx == AMDGV_PF_IDX)
		ret = amdgv_get_pf_regions(ras_core, output_data);
	else
		ret = amdgv_get_vf_regions(ras_core, input_data->vf_idx, output_data);

	if (!ret) {
		output_data->version = 0;
		cmd->output_size = sizeof(struct ras_cmd_fb_regions_rsp);
	}

	return ret;
}

static int amdgv_ras_get_bad_pages(struct ras_core_context *ras_core,
			struct ras_cmd_ctx *cmd, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct ras_cmd_bad_pages_info_rsp *output_data =
			(struct ras_cmd_bad_pages_info_rsp *)cmd->output_buff_raw;
	struct ras_cmd_param *vf_cmd = (struct ras_cmd_param *)data;
	struct ras_cmd_bad_page_record *rc;
	struct amdgv_vf_device *entry;
	int ret, i;

	ret = rascore_handle_cmd(ras_core, cmd, data);
	if (ret)
		return ret;

	if (check_vf_ras_cmd(ras_core, vf_cmd)) {
		for (i = 0; i < output_data->bp_in_group; i++) {
			rc = &output_data->records[i];
			if (vf_cmd->idx_vf ==
			    amdgv_umc_calc_retired_page_vf_slot(adapt,
					RAS_PFN_TO_ADDR(rc->retired_page))) {
				entry = &adapt->array_vf[vf_cmd->idx_vf];
				rc->retired_page -=
					RAS_ADDR_TO_PFN(MBYTES_TO_BYTES(entry->fb_offset));
			} else {
				oss_memset(rc, 0, sizeof(*rc));
			}
		}
	}

	return ret;
}

static int amdgv_ras_get_all_block_ecc_info(struct ras_core_context *ras_core,
				struct ras_cmd_ctx *cmd, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct ras_cmd_blocks_ecc_rsp *output_data =
			(struct ras_cmd_blocks_ecc_rsp *)cmd->output_buff_raw;
	struct ras_cmd_param *vf_cmd = (struct ras_cmd_param *)data;
	struct vf_ras_lifespan_data *vf_ras;
	struct ras_cmd_block_ecc *baseline;
	struct ras_ecc_count ecc;
	uint64_t ras_caps;
	uint32_t blk;

	if (cmd->input_size != sizeof(struct ras_cmd_blocks_ecc_req))
		return RAS_CMD__ERROR_INVALID_INPUT_SIZE;

	if (!ras_mgr)
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	vf_ras = vf_cmd ? &ras_mgr->array_vf[vf_cmd->idx_vf] : NULL;
	ras_caps = ras_core_get_ras_caps(ras_core);

	for (blk = 0; blk < MAX_RAS_BLOCK_NUM && blk < RAS_BLOCK_ID__LAST; blk++) {
		/* Check VF ras block permissions */
		if (vf_cmd && !(ras_caps & BIT_ULL(blk)))
			continue;

		oss_memset(&ecc, 0, sizeof(ecc));
		if (ras_core_query_block_ecc_data(ras_core, blk, &ecc, false))
			return RAS_CMD__ERROR_GENERIC;

		output_data->blocks[blk].ce_count = ecc.total_ce_count;
		output_data->blocks[blk].ue_count = ecc.total_ue_count;
		output_data->blocks[blk].de_count = ecc.total_de_count;

		if (vf_ras) {
			baseline = &vf_ras->ecc_count_baseline[blk];

			output_data->blocks[blk].ce_count =
				output_data->blocks[blk].ce_count > baseline->ce_count ?
				output_data->blocks[blk].ce_count - baseline->ce_count : 0;
			output_data->blocks[blk].ue_count =
				output_data->blocks[blk].ue_count > baseline->ue_count ?
				output_data->blocks[blk].ue_count - baseline->ue_count : 0;
			output_data->blocks[blk].de_count =
				output_data->blocks[blk].de_count > baseline->de_count ?
				output_data->blocks[blk].de_count - baseline->de_count : 0;
		}
	}

	cmd->output_size = sizeof(struct ras_cmd_blocks_ecc_rsp);

	return RAS_CMD__SUCCESS;
}

static int amdgv_ras_set_cmd_auto_update(struct ras_core_context *ras_core,
				struct ras_cmd_ctx *cmd, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct ras_cmd_param *vf_cmd = (struct ras_cmd_param *)data;
	struct ras_cmd_auto_update_req *input_data =
			(struct ras_cmd_auto_update_req *) cmd->input_buff_raw;
	struct auto_update_cmd *auto_cmd = NULL;
	struct auto_update_cmd rcmd = {0};
	int ret = RAS_CMD__SUCCESS;

	if (!ras_mgr || !vf_cmd || (vf_cmd->idx_vf >= AMDGV_MAX_VF_NUM))
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	if (cmd->input_size != sizeof(struct ras_cmd_auto_update_req))
		return RAS_CMD__ERROR_INVALID_INPUT_SIZE;

	rcmd.cmd_id = input_data->cmd_id;
	rcmd.addr = input_data->addr;
	rcmd.len = input_data->len;
	rcmd.data = vf_cmd->data;

	auto_cmd = amdgv_ras_add_vf_cmd_to_auto_list(adapt,
			vf_cmd->idx_vf, &rcmd, input_data->mode);
	if (auto_cmd && input_data->mode) {
		if (amdgv_ras_handle_vf_cmd(adapt,
			vf_cmd->idx_vf, auto_cmd->addr, auto_cmd->len))
			auto_cmd->need_update = true;
		else
			auto_cmd->need_update = false;
	}

	cmd->output_size = sizeof(struct ras_cmd_auto_update_rsp);
	return ret;
}

static int amdgv_ras_get_cper_records(struct ras_core_context *ras_core,
			struct ras_cmd_ctx *cmd, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct ras_cmd_cper_record_req *req =
		(struct ras_cmd_cper_record_req *)cmd->input_buff_raw;
	struct ras_cmd_param *vf_cmd = (struct ras_cmd_param *)data;
	struct vf_ras_lifespan_data *vf_ras;
	uint64_t user_addr = 0;
	uint8_t *buf_ptr = NULL;
	int ret;

	if (!ras_mgr)
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	if (cmd->input_size != sizeof(struct ras_cmd_cper_record_req))
		return RAS_CMD__ERROR_INVALID_INPUT_SIZE;

	if (!req->buf_size || !req->buf_ptr || !req->cper_num)
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	if (oss_access_ok(u64_to_user_ptr(req->buf_ptr), req->buf_size)) {
		user_addr = req->buf_ptr;
		buf_ptr = oss_zalloc(req->buf_size);
		if (!buf_ptr)
			return RAS_CMD__ERROR_GENERIC;

		req->buf_ptr = (uintptr_t)buf_ptr;
	}

	vf_ras = vf_cmd ? &ras_mgr->array_vf[vf_cmd->idx_vf] : NULL;
	if (vf_ras)
		req->cper_start_id = req->cper_start_id > vf_ras->cper_ptr_record.start_rptr ?
			req->cper_start_id : vf_ras->cper_ptr_record.start_rptr;

	ret = rascore_handle_cmd(ras_core, cmd, data);
	if (ret) {
		oss_free(buf_ptr);
		return ret;
	}

	if (user_addr) {
		if (oss_copy_to_user(u64_to_user_ptr(user_addr),
				(void *)(uintptr_t)req->buf_ptr, req->buf_size))
			ret = RAS_CMD__ERROR_GENERIC;
		req->buf_ptr = user_addr;
		oss_free(buf_ptr);
	}

	return ret;
}

static int amdgv_ras_get_batch_trace_records(struct ras_core_context *ras_core,
			struct ras_cmd_ctx *cmd, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct ras_cmd_batch_trace_record_req *req =
		(struct ras_cmd_batch_trace_record_req *)cmd->input_buff_raw;
	struct ras_cmd_param *vf_cmd = (struct ras_cmd_param *)data;
	struct vf_ras_lifespan_data *vf_ras;
	int ret;

	if (!ras_mgr)
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	if (cmd->input_size != sizeof(struct ras_cmd_batch_trace_record_req))
		return RAS_CMD__ERROR_INVALID_INPUT_SIZE;

	if (!req->batch_num)
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	vf_ras = vf_cmd ? &ras_mgr->array_vf[vf_cmd->idx_vf] : NULL;
	if (vf_ras)
		req->start_batch_id = req->start_batch_id > vf_ras->cper_ptr_record.start_rptr ?
			req->start_batch_id : vf_ras->cper_ptr_record.start_rptr;

	ret = rascore_handle_cmd(ras_core, cmd, data);
	if (ret)
		return ret;

	return ret;
}

static int amdgv_ras_check_address_validity(struct ras_core_context *ras_core,
	struct ras_cmd_ctx *cmd, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct ras_cmd_address_check_req *input_data =
			(struct ras_cmd_address_check_req *)cmd->input_buff_raw;
	struct ras_cmd_address_check_rsp *output_data =
			(struct ras_cmd_address_check_rsp *)cmd->output_buff_raw;
	struct ras_cmd_param *vf_cmd = (struct ras_cmd_param *)data;
	struct amdgv_vf_device *entry;
	struct eeprom_umc_record record = {0};
	uint64_t address;
	uint64_t *page_pfns = NULL;
	uint32_t nr_page_pfns;
	uint32_t result = 0, vf_idx;
	int i, count, ret;

	if ((cmd->input_size != sizeof(*input_data)) ||
		!input_data->address)
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	ret = ras_umc_alloc_row_pages(ras_core, &page_pfns, &nr_page_pfns);
	if (ret)
		return RAS_CMD__ERROR_GENERIC;

	address = input_data->address;

	/* Adjust address for PF */
	if ((input_data->vf_idx < adapt->num_vf) ||
			check_vf_ras_cmd(ras_core, vf_cmd)) {
		vf_idx = vf_cmd ? vf_cmd->idx_vf : input_data->vf_idx;
		entry = &adapt->array_vf[vf_idx];
		address += MBYTES_TO_BYTES(entry->fb_offset);
	}

	record.cur_nps_retired_row_pfn = RAS_ADDR_TO_PFN(address);
	record.cur_nps = ras_core_get_curr_nps_mode(ras_core);

	count = ras_umc_convert_record_to_row_pages(ras_core,
				&record, page_pfns, nr_page_pfns);
	if (count <= 0) {
		ret = RAS_CMD__ERROR_DRV_INIT_FAIL;
		goto out;
	}

	for (i = 0; i < count; i++) {
		if (amdgv_memmgr_check_critical_address(adapt,
				page_pfns[i] << OSS_GPU_PAGE_SHIFT)) {
			result = RAS_CMD__ERROR_ACCESS_DENIED;
			break;
		}
	}

	output_data->result = result;
	output_data->version = 0;
	cmd->output_size = sizeof(struct ras_cmd_address_check_rsp);

	ret = RAS_CORE_OK;

out:
	ras_umc_free_row_pages(ras_core, page_pfns);
	return ret;
}

static int amdgv_ras_convert_retired_address(struct ras_core_context *ras_core,
			struct ras_cmd_ctx *cmd, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct ras_cmd_convert_retired_address_req *req =
		(struct ras_cmd_convert_retired_address_req *)cmd->input_buff_raw;
	struct ras_cmd_convert_retired_address_rsp *output_data =
		(struct ras_cmd_convert_retired_address_rsp *)cmd->output_buff_raw;
	struct ras_cmd_param *vf_cmd = (struct ras_cmd_param *)data;
	struct eeprom_umc_record record = {0};
	struct amdgv_vf_device *entry;
	uint32_t valid_count = 0;
	int count;
	int ret = 0, i;
	uint64_t guest_addr, vf_fb_size;

	if (cmd->input_size != sizeof(struct ras_cmd_convert_retired_address_req))
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	/* convert GPA to SPA */
	entry = &adapt->array_vf[vf_cmd->idx_vf];
	req->address += MBYTES_TO_BYTES(entry->fb_offset);
	vf_fb_size = MBYTES_TO_BYTES(entry->fb_size);

	/* convert address to nps pages */
	record.cur_nps_retired_row_pfn = RAS_ADDR_TO_PFN(req->address);
	record.cur_nps = ras_core_get_curr_nps_mode(ras_core);
	count = ras_umc_convert_record_to_row_pages(ras_core, &record,
			output_data->retired_addr, RAS_CMD_MAX_RETIRED_ADDR_COUNT);
	if (count <= 0) {
		RAS_DEV_ERR(ras_core->dev, "Failed to convert retired address: 0x%llx, %d\n",
			req->address, count);
		return RAS_CMD__ERROR_DRV_INIT_FAIL;
	}

	cmd->output_size = sizeof(struct ras_cmd_convert_retired_address_rsp);
	output_data->retired_count = 0;

	/* Convert back to guest nps pages */
	if (check_vf_ras_cmd(ras_core, vf_cmd)) {
		for (i = 0; i < count; i++) {
			guest_addr = RAS_PFN_TO_ADDR(output_data->retired_addr[i]) - MBYTES_TO_BYTES(entry->fb_offset);

			/* Check if the converted guest physical address is within the guest's physical address space */
			if (guest_addr < vf_fb_size) {
				/* Save valid address */
				output_data->retired_addr[valid_count] = guest_addr;

				valid_count++;
			} else {
				RAS_DEV_WARN(ras_core->dev, "Converted guest physical address 0x%llx exceeds VF[%d] FB size 0x%llx, skipping\n",
					guest_addr, vf_cmd->idx_vf, vf_fb_size);
			}
		}
		/* Update retired_count to reflect only valid addresses */
		output_data->retired_count = valid_count;
	}

	return ret;
}

static int amdgv_ras_reset_all_error_counts(struct ras_core_context *ras_core,
				struct ras_cmd_ctx *cmd, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	uint32_t idx_vf;
	int ret;

	ret = rascore_handle_cmd(ras_core, cmd, data);

	/* Baselines are tracked per VF only */
	for (idx_vf = 0; idx_vf < AMDGV_MAX_VF_NUM; idx_vf++)
		amdgv_ras_mgr_vf_ecc_count_init(adapt, idx_vf);

	return ret;
}

static int amdgv_ras_get_cper_snapshot(struct ras_core_context *ras_core,
			struct ras_cmd_ctx *cmd, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct ras_cmd_cper_snapshot_rsp *rsp;
	struct ras_cmd_param *vf_cmd = (struct ras_cmd_param *)data;
	struct vf_ras_lifespan_data *vf_ras;
	uint64_t baseline;
	int ret;

	if (!ras_mgr)
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	if (cmd->input_size != sizeof(struct ras_cmd_cper_snapshot_req))
		return RAS_CMD__ERROR_INVALID_INPUT_SIZE;

	ret = rascore_handle_cmd(ras_core, cmd, data);
	if (ret)
		return ret;

	if (vf_cmd) {
		vf_ras = &ras_mgr->array_vf[vf_cmd->idx_vf];
		baseline = vf_ras->cper_ptr_record.start_rptr;

		rsp = (struct ras_cmd_cper_snapshot_rsp *)cmd->output_buff_raw;
		if (rsp->start_cper_id < baseline) {
			if (baseline >= rsp->latest_cper_id) {
				rsp->total_cper_num = 0;
			} else {
				rsp->total_cper_num = (uint32_t)(rsp->latest_cper_id - baseline);
			}
			rsp->start_cper_id = baseline;
		}
	}

	return ret;
}

static int amdgv_ras_get_batch_trace_snapshot(struct ras_core_context *ras_core,
			struct ras_cmd_ctx *cmd, void *data)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)ras_core->dev;
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct ras_cmd_batch_trace_snapshot_rsp *rsp;
	struct ras_cmd_param *vf_cmd = (struct ras_cmd_param *)data;
	struct vf_ras_lifespan_data *vf_ras;
	uint64_t baseline;
	int ret;

	if (!ras_mgr)
		return RAS_CMD__ERROR_INVALID_INPUT_DATA;

	if (cmd->input_size != sizeof(struct ras_cmd_batch_trace_snapshot_req))
		return RAS_CMD__ERROR_INVALID_INPUT_SIZE;

	ret = rascore_handle_cmd(ras_core, cmd, data);
	if (ret)
		return ret;

	if (vf_cmd) {
		vf_ras = &ras_mgr->array_vf[vf_cmd->idx_vf];
		baseline = vf_ras->cper_ptr_record.start_rptr;

		rsp = (struct ras_cmd_batch_trace_snapshot_rsp *)cmd->output_buff_raw;
		if (rsp->start_batch_id < baseline) {
			if (baseline >= rsp->latest_batch_id) {
				rsp->total_batch_num = 0;
			} else {
				rsp->total_batch_num = (uint32_t)(rsp->latest_batch_id - baseline);
			}
			rsp->start_batch_id = baseline;
		}
	}

	return ret;
}

static struct ras_cmd_func_map amdgv_ras_cmd_maps[] = {
	{RAS_CMD__INJECT_ERROR, amdgv_ras_inject_error},
	{RAS_CMD__GET_BLOCK_ECC_STATUS, amdgv_ras_get_block_ecc_info},
	{RAS_CMD__RAS_TA_LOAD, amdgv_ras_load_ta},
	{RAS_CMD__RAS_TA_UNLOAD, amdgv_ras_unload_ta},
	{RAS_CMD__GET_SAFE_FB_ADDRESS_RANGES, amdgv_ras_get_ras_safe_fb_addr_ranges},
	{RAS_CMD__TRANSLATE_FB_ADDRESS, amdgv_ras_translate_fb_address},
	{RAS_CMD__GET_LINK_TOPOLOGY, amdgv_ras_get_link_topology},
	{RAS_CMD__GET_FB_REGIONS, amdgv_ras_fb_regions},
	{RAS_CMD__GET_BAD_PAGES, amdgv_ras_get_bad_pages},
	{RAS_CMD__GET_ALL_BLOCK_ECC_STATUS, amdgv_ras_get_all_block_ecc_info},
	{RAS_CMD__SET_CMD_AUTO_UPDATE, amdgv_ras_set_cmd_auto_update},
	{RAS_CMD__GET_CPER_SNAPSHOT, amdgv_ras_get_cper_snapshot},
	{RAS_CMD__GET_CPER_RECORD, amdgv_ras_get_cper_records},
	{RAS_CMD__GET_BATCH_TRACE_SNAPSHOT, amdgv_ras_get_batch_trace_snapshot},
	{RAS_CMD__GET_BATCH_TRACE_RECORD, amdgv_ras_get_batch_trace_records},
	{RAS_CMD__CHECK_ADDRESS_VALIDITY, amdgv_ras_check_address_validity},
	{RAS_CMD__CONVERT_RETIRED_ADDRESS, amdgv_ras_convert_retired_address},
	{RAS_CMD__RESET_ALL_ERROR_COUNTS, amdgv_ras_reset_all_error_counts},
};

static int amdgv_ras_handle_cmd(struct ras_core_context *ras_core,
			struct ras_cmd_ctx *cmd, void *data)
{
	struct ras_cmd_func_map *ras_cmd = NULL;
	int i, res;

	for (i = 0; i < ARRAY_SIZE(amdgv_ras_cmd_maps); i++) {
		if (cmd->cmd_id == amdgv_ras_cmd_maps[i].cmd_id) {
			ras_cmd = &amdgv_ras_cmd_maps[i];
			break;
		}
	}

	if (ras_cmd)
		res = ras_cmd->func(ras_core, cmd, data);
	else
		res = RAS_CMD__ERROR_UKNOWN_CMD;

	return res;
}

int amdgv_ras_submit_cmd(struct ras_core_context *ras_core,
			struct ras_cmd_ctx *cmd, void *data)
{
	struct ras_core_context *cmd_core = ras_core;
	int timeout = 60;
	int res;

	cmd->cmd_res = RAS_CMD__ERROR_INVALID_CMD;
	cmd->output_size = 0;

	if (!ras_core_is_enabled(cmd_core))
		return RAS_CMD__ERROR_ACCESS_DENIED;

	while (ras_core_gpu_in_reset(cmd_core)) {
		oss_msleep(1000);
		if (!timeout--)
			return RAS_CMD__ERROR_TIMEOUT;
	}

	res = amdgv_ras_handle_cmd(cmd_core, cmd, data);
	if (res == RAS_CMD__ERROR_UKNOWN_CMD)
		res = rascore_handle_cmd(cmd_core, cmd, data);

	cmd->cmd_res = res;

	if (cmd->output_size > cmd->output_buf_size) {
		RAS_DEV_ERR(cmd_core->dev,
			"Output size 0x%x exceeds output buffer size 0x%x!\n",
			cmd->output_size, cmd->output_buf_size);
		return RAS_CMD__SUCCESS_EXEED_BUFFER;
	}

	return RAS_CMD__SUCCESS;
}

int amdgv_ras_cmd_ioctl_handler(struct ras_core_context *ras_core,
			uint8_t *cmd_buf, uint32_t buf_size)
{
	struct ras_cmd_ctx *cmd = (struct ras_cmd_ctx *)cmd_buf;
	struct ras_core_context *cmd_core = NULL;
	struct ras_cmd_dev_handle *cmd_handle = NULL;

	if (buf_size < sizeof(*cmd))
		return RAS_CMD__ERROR_INVALID_INPUT_SIZE;

	cmd->cmd_res = RAS_CMD__ERROR_INVALID_CMD;
	cmd->output_size = 0;
	cmd->output_buf_size = buf_size - sizeof(*cmd);

	if (cmd->cmd_id == RAS_CMD__QUERY_INTERFACE_INFO) {
		cmd->cmd_res = amdgv_ras_query_interface_info(ras_core, cmd);
	} else if (cmd->cmd_id == RAS_CMD__GET_DEVICES_INFO) {
		cmd->cmd_res = amdgv_ras_get_devices_info(ras_core, cmd);
	} else {
		cmd_handle = (struct ras_cmd_dev_handle *)cmd->input_buff_raw;
		cmd_core = ras_cmd_get_ras_core(cmd_handle->dev_handle);
		if (!cmd_core)
			return RAS_CMD__ERROR_INVALID_INPUT_DATA;

		cmd->cmd_res = amdgv_ras_submit_cmd(cmd_core, cmd, NULL);
	}

	if (cmd->output_size > cmd->output_buf_size) {
		RAS_INFO("Insufficient command buffer size 0x%x!\n", buf_size);
		return RAS_CMD__SUCCESS_EXEED_BUFFER;
	}

	return 0;
}

int amdgv_ras_cmd_handle_vf_cmd(struct ras_core_context *ras_core,
		uint32_t idx_vf, struct ras_cmd_ctx *cmd)
{
	struct ras_cmd_param cmd_param = {
		.idx_vf = idx_vf,
	};

	switch (cmd->cmd_id) {
	case RAS_CMD__GET_BLOCK_ECC_STATUS:
	case RAS_CMD__GET_BAD_PAGES:
	case RAS_CMD__GET_CPER_SNAPSHOT:
	case RAS_CMD__GET_CPER_RECORD:
	case RAS_CMD__GET_BATCH_TRACE_SNAPSHOT:
	case RAS_CMD__GET_BATCH_TRACE_RECORD:
	case RAS_CMD__SET_CMD_AUTO_UPDATE:
	case RAS_CMD__GET_ALL_BLOCK_ECC_STATUS:
	case RAS_CMD__CHECK_ADDRESS_VALIDITY:
	case RAS_CMD__CONVERT_RETIRED_ADDRESS:
		break;
	default:
		return RAS_CMD__ERROR_ACCESS_DENIED;
	}

	cmd->cmd_res = amdgv_ras_submit_cmd(ras_core, cmd, &cmd_param);;

	return 0;
}

struct auto_update_cmd *amdgv_ras_add_vf_cmd_to_auto_list(struct amdgv_adapter *adapt,
			uint32_t idx_vf, struct auto_update_cmd *rcmd, uint32_t mode)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct auto_update_cmd *auto_cmd = NULL, *tmp;
	struct vf_auto_cmd_mgr *cmd_mgr;
	bool found = false;

	if (idx_vf >= AMDGV_MAX_VF_NUM)
		return NULL;

	cmd_mgr = &ras_mgr->vf_auto_cmd_mgr[idx_vf];
	oss_mutex_lock(&cmd_mgr->cmd_lock);
	oss_list_for_each_entry_safe(auto_cmd, tmp,
		&cmd_mgr->cmd_list, struct auto_update_cmd, node) {
		if (auto_cmd->cmd_id == rcmd->cmd_id) {
			if (mode) {
				auto_cmd->addr = rcmd->addr;
				auto_cmd->len = rcmd->len;
			} else {
				oss_list_del(&auto_cmd->node);
				oss_free(auto_cmd);
				auto_cmd = NULL;
				cmd_mgr->list_count--;
			}
			found = true;
			break;
		}
	}

	if (!found) {
		auto_cmd = oss_zalloc(sizeof(*auto_cmd));
		if (auto_cmd) {
			oss_memset(auto_cmd, 0, sizeof(*auto_cmd));
			auto_cmd->data = rcmd->data;
			auto_cmd->cmd_id = rcmd->cmd_id;
			auto_cmd->addr = rcmd->addr;
			auto_cmd->len = rcmd->len;
			oss_list_add_tail(&auto_cmd->node, &cmd_mgr->cmd_list);
			cmd_mgr->list_count++;
		}
	}
	oss_mutex_unlock(&cmd_mgr->cmd_lock);

	return auto_cmd;
}

int amdgv_ras_cmd_update_auto_list(struct amdgv_adapter *adapt)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct vf_auto_cmd_mgr *cmd_mgr;
	struct auto_update_cmd *auto_cmd;
	int ret = 0, idx_vf;

	for (idx_vf = 0; idx_vf < AMDGV_MAX_VF_NUM; idx_vf++) {
		cmd_mgr = &ras_mgr->vf_auto_cmd_mgr[idx_vf];
		oss_mutex_lock(&cmd_mgr->cmd_lock);
		if (!oss_list_empty(&cmd_mgr->cmd_list)) {
			oss_list_for_each_entry(auto_cmd,
				&cmd_mgr->cmd_list, struct auto_update_cmd, node) {
				if (auto_cmd->disabled)
					continue;

				if (!cmd_mgr->need_update_all && !auto_cmd->need_update)
					continue;

				if (!is_full_access_vf(idx_vf)) {
					auto_cmd->need_update = true;
					continue;
				}

				if (amdgv_ras_handle_vf_cmd(adapt,
						idx_vf, auto_cmd->addr, auto_cmd->len))
					auto_cmd->exe_err_cnt++;
				else
					auto_cmd->exe_err_cnt = 0;

				if (!auto_cmd->exe_err_cnt)
					auto_cmd->need_update = false;
				else if (auto_cmd->exe_err_cnt > ALLOWED_MAX_FAILURE_NUM)
					auto_cmd->disabled = true;
				else
					auto_cmd->need_update = true;

				if (auto_cmd->disabled)
					AMDGV_WARN("Auto cmd %u in VF%u is disabled\n",
							auto_cmd->cmd_id, idx_vf);
			}
		}
		cmd_mgr->need_update_all = false;
		oss_mutex_unlock(&cmd_mgr->cmd_lock);
	}

	return ret;
}

int amdgv_ras_cmd_set_auto_list(struct amdgv_adapter *adapt, bool enable)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct vf_auto_cmd_mgr *cmd_mgr;
	int idx_vf;

	for (idx_vf = 0; idx_vf < AMDGV_MAX_VF_NUM; idx_vf++) {
		cmd_mgr = &ras_mgr->vf_auto_cmd_mgr[idx_vf];
		oss_mutex_lock(&cmd_mgr->cmd_lock);
		cmd_mgr->need_update_all = enable;
		oss_mutex_unlock(&cmd_mgr->cmd_lock);
	}

	return 0;
}

int amdgv_ras_cmd_clear_vf_auto_list(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	struct amdgv_ras_mgr *ras_mgr = amdgv_ras_mgr_get_context(adapt);
	struct auto_update_cmd *auto_cmd = NULL, *tmp;
	struct vf_auto_cmd_mgr *cmd_mgr;

	if (idx_vf >= AMDGV_MAX_VF_NUM)
		return -RAS_CORE_EINVAL;

	cmd_mgr = &ras_mgr->vf_auto_cmd_mgr[idx_vf];
	oss_mutex_lock(&cmd_mgr->cmd_lock);
	oss_list_for_each_entry_safe(auto_cmd, tmp,
		&cmd_mgr->cmd_list, struct auto_update_cmd, node) {
		oss_list_del(&auto_cmd->node);
		oss_free(auto_cmd);
		cmd_mgr->list_count--;
	}
	cmd_mgr->need_update_all = false;
	oss_mutex_unlock(&cmd_mgr->cmd_lock);

	return 0;
}