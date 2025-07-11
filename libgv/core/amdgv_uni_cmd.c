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
 * THE SOFTWARE
 */

#include "amdgv_basetypes.h"
#include "amdgv_cmd_uni_def.h"
#include "amdgv_gpumon.h"
#include "amdgv_oss_wrapper.h"

typedef uint8_t (*cmd_func)(amdgv_dev_t dev, struct amdgv_uni_cmd *cmd);

typedef struct amdgv_cmd_func_map {
	uint32_t cmd_id;
	cmd_func func;
} amdgv_cmd_func_map;

static enum amdgv_smi_ras_error_type amdgv_ras_error_type_to_ta_error_type(enum amdgv_ras_error_type err_type)
{
	switch (err_type) {
	case AMDGV_RAS_TYPE_ERROR__NONE:
		return AMDGV_SMI_RAS_ERROR__NONE;
	case AMDGV_RAS_TYPE_ERROR__PARITY:
		return AMDGV_SMI_RAS_ERROR__PARITY;
	case AMDGV_RAS_TYPE_ERROR__SINGLE_CORRECTABLE:
		return AMDGV_SMI_RAS_ERROR__SINGLE_CORRECTABLE;
	case AMDGV_RAS_TYPE_ERROR__MULTI_UNCORRECTABLE:
		return AMDGV_SMI_RAS_ERROR__MULTI_UNCORRECTABLE;
	case AMDGV_RAS_TYPE_ERROR__POISON:
		return AMDGV_SMI_RAS_ERROR__POISON;
	default:
		return -1;
	}
}

static enum amdgv_smi_ras_block amdgv_block_to_ta_block(enum amdgv_ras_block block_id)
{
	switch (block_id) {
	case AMDGV_RAS_BLOCK__UMC:
		return AMDGV_SMI_RAS_BLOCK__UMC;
	case AMDGV_RAS_BLOCK__SDMA:
		return AMDGV_SMI_RAS_BLOCK__SDMA;
	case AMDGV_RAS_BLOCK__GFX:
		return AMDGV_SMI_RAS_BLOCK__GFX;
	case AMDGV_RAS_BLOCK__MMHUB:
		return AMDGV_SMI_RAS_BLOCK__MMHUB;
	case AMDGV_RAS_BLOCK__ATHUB:
		return AMDGV_SMI_RAS_BLOCK__ATHUB;
	case AMDGV_RAS_BLOCK__PCIE_BIF:
		return AMDGV_SMI_RAS_BLOCK__PCIE_BIF;
	case AMDGV_RAS_BLOCK__HDP:
		return AMDGV_SMI_RAS_BLOCK__HDP;
	case AMDGV_RAS_BLOCK__XGMI_WAFL:
		return AMDGV_SMI_RAS_BLOCK__XGMI_WAFL;
	case AMDGV_RAS_BLOCK__DF:
		return AMDGV_SMI_RAS_BLOCK__DF;
	case AMDGV_RAS_BLOCK__SMN:
		return AMDGV_SMI_RAS_BLOCK__SMN;
	case AMDGV_RAS_BLOCK__SEM:
		return AMDGV_SMI_RAS_BLOCK__SEM;
	case AMDGV_RAS_BLOCK__MP0:
		return AMDGV_SMI_RAS_BLOCK__MP0;
	case AMDGV_RAS_BLOCK__MP1:
		return AMDGV_SMI_RAS_BLOCK__MP1;
	case AMDGV_RAS_BLOCK__FUSE:
		return AMDGV_SMI_RAS_BLOCK__FUSE;
	default:
		return -1;
	}
}

static enum amdgv_smi_ras_error_type amdgv_ta_error_type_to_ras_error_type(enum amdgv_smi_ras_error_type err_type)
{
	switch (err_type) {
	case AMDGV_SMI_RAS_ERROR__NONE:
		return AMDGV_RAS_TYPE_ERROR__NONE;
	case AMDGV_SMI_RAS_ERROR__PARITY:
		return AMDGV_RAS_TYPE_ERROR__PARITY;
	case AMDGV_SMI_RAS_ERROR__SINGLE_CORRECTABLE:
		return AMDGV_RAS_TYPE_ERROR__SINGLE_CORRECTABLE;
	case AMDGV_SMI_RAS_ERROR__MULTI_UNCORRECTABLE:
		return AMDGV_RAS_TYPE_ERROR__MULTI_UNCORRECTABLE;
	case AMDGV_SMI_RAS_ERROR__POISON:
		return AMDGV_RAS_TYPE_ERROR__POISON;
	default:
		return -1;
	}
}

static void __amdgv_update_bad_page_info(struct amdgv_cmd_bad_page_record *amdgv_record,
	struct amdgv_smi_ras_eeprom_table_record *record)
{
	amdgv_record->retired_page = record->retired_page;
	amdgv_record->ts = record->ts;
	amdgv_record->err_type = amdgv_ta_error_type_to_ras_error_type(record->err_type);
	amdgv_record->mem_channel = record->mem_channel;
	amdgv_record->mcumc_id = record->mcumc_id;
	amdgv_record->address = record->address;
	amdgv_record->bank = record->bank;
}

static uint32_t __amdgv_get_bad_pages(amdgv_dev_t *adev, uint32_t group_index,
	 struct amdgv_cmd_bad_pages_info *output_data)
{
	struct amdgv_smi_ras_eeprom_table_record record;
	uint32_t i = 0, bp_cnt = 0, group_cnt = 0;

	output_data->bp_in_group = 0;
	output_data->group_index = 0;

	amdgv_gpumon_get_bad_page_record_count(adev, &bp_cnt);
	if (bp_cnt) {
		output_data->group_index = group_index;
		group_cnt = bp_cnt / AMDGV_CMD_MAX_BAD_PAGES_PER_GROUP
			+ ((bp_cnt % AMDGV_CMD_MAX_BAD_PAGES_PER_GROUP) ? 1 : 0);

		if (group_index >= group_cnt)
			return AMDGV_CMD__ERROR_INVALID_INPUT;

		i = group_index * AMDGV_CMD_MAX_BAD_PAGES_PER_GROUP;
		for (; i < bp_cnt && output_data->bp_in_group < AMDGV_CMD_MAX_BAD_PAGES_PER_GROUP; i++) {
			if (amdgv_gpumon_get_bad_page_info(adev, i, &record))
				return AMDGV_CMD__ERROR_GENERIC;

			__amdgv_update_bad_page_info(&output_data->records[i % AMDGV_CMD_MAX_BAD_PAGES_PER_GROUP], &record);
			output_data->bp_in_group++;
		}
	}
	output_data->bp_total_cnt = bp_cnt;
	return AMDGV_CMD__SUCCESS;
}

static uint8_t amdgv_get_safe_fb_addr_ranges(amdgv_dev_t adev, struct amdgv_uni_cmd *cmd)
{
	struct amdgv_cmd_ras_safe_fb_address_ranges_rsp *rsp_buff =
			(struct amdgv_cmd_ras_safe_fb_address_ranges_rsp *)cmd->output_buff_raw;
	struct amdgv_gpumon_ras_safe_fb_address_ranges *libgv_safe_ranges;
	int ret, i;

	if (cmd->input_size != sizeof(struct amdgv_cmd_dev_handle) ||
			cmd->version != AMDGV_CMD_VERSION_V1 || !adev)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	libgv_safe_ranges = oss_alloc_memory(sizeof(*libgv_safe_ranges));
	if (!libgv_safe_ranges)
		return AMDGV_CMD__ERROR_GENERIC;

	ret = amdgv_gpumon_get_ras_safe_fb_addr_ranges((amdgv_dev_t)adev, libgv_safe_ranges);
	if (ret)
		return AMDGV_CMD__ERROR_GENERIC;

	if (libgv_safe_ranges->num_ranges > AMDGV_RAS_MAX_NUM_SAFE_RANGES)
		rsp_buff->num_ranges = AMDGV_RAS_MAX_NUM_SAFE_RANGES;
	else
		rsp_buff->num_ranges = libgv_safe_ranges->num_ranges;

	for (i = 0; i < libgv_safe_ranges->num_ranges; i++) {
		rsp_buff->range[i].start = libgv_safe_ranges->range[i].start;
		rsp_buff->range[i].size = libgv_safe_ranges->range[i].size;
	}

	cmd->output_size = sizeof(struct amdgv_cmd_ras_safe_fb_address_ranges_rsp);

	oss_free_memory(libgv_safe_ranges);
	return AMDGV_CMD__SUCCESS;
}

static enum amdgv_gpumon_fb_addr_type amdgv_fb_addr_type_to_gpumon_type(enum amdgv_fb_addr_type src_type)
{
	switch (src_type) {
	case AMDGV_FB_ADDR_SOC_PHY:
		return AMDGV_GPUMON_FB_ADDR_SOC_PHY;
	case AMDGV_FB_ADDR_BANK:
		return AMDGV_GPUMON_FB_ADDR_BANK;
	case AMDGV_FB_ADDR_VF_PHY:
		return AMDGV_GPUMON_FB_ADDR_VF_PHY;
	default:
		return AMDGV_GPUMON_FB_ADDR_SOC_PHY;
	}

	return AMDGV_GPUMON_FB_ADDR_SOC_PHY;
}

static uint8_t amdgv_translate_fb_address(amdgv_dev_t adev, struct amdgv_uni_cmd *cmd)
{
	struct amdgv_cmd_translate_fb_address_req *req_buff =
			(struct amdgv_cmd_translate_fb_address_req *)cmd->input_buff_raw;
	struct amdgv_cmd_translate_fb_address_rsp *rsp_buff =
			(struct amdgv_cmd_translate_fb_address_rsp *)cmd->output_buff_raw;
	union amdgv_gpumon_translate_fb_address translated_addr_in = { 0 };
	union amdgv_gpumon_translate_fb_address translated_addr_out = { 0 };
	int ret = AMDGV_CMD__ERROR_GENERIC;

	if (cmd->input_size != sizeof(struct amdgv_cmd_translate_fb_address_req) ||
			cmd->version != AMDGV_CMD_VERSION_V1)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	if (req_buff->src_addr_type == AMDGV_FB_ADDR_SOC_PHY) {
		translated_addr_in.soc_phy_addr = req_buff->soc_phy_addr;

		ret = amdgv_gpumon_translate_fb_address(adev,
				AMDGV_GPUMON_FB_ADDR_SOC_PHY,
				amdgv_fb_addr_type_to_gpumon_type(req_buff->dest_addr_type),
				translated_addr_in,
				&translated_addr_out);
	} else if (req_buff->src_addr_type == AMDGV_FB_ADDR_BANK) {
		translated_addr_in.bank_addr.stack_id = req_buff->bank_addr.stack_id;
		translated_addr_in.bank_addr.bank_group = req_buff->bank_addr.bank_group;
		translated_addr_in.bank_addr.bank = req_buff->bank_addr.bank;
		translated_addr_in.bank_addr.row = req_buff->bank_addr.row;
		translated_addr_in.bank_addr.column = req_buff->bank_addr.column;
		translated_addr_in.bank_addr.channel = req_buff->bank_addr.channel;
		translated_addr_in.bank_addr.subchannel = req_buff->bank_addr.subchannel;

		ret = amdgv_gpumon_translate_fb_address(
				adev,
				AMDGV_GPUMON_FB_ADDR_BANK,
				amdgv_fb_addr_type_to_gpumon_type(req_buff->dest_addr_type),
				translated_addr_in,
				&translated_addr_out);
	} else if (req_buff->src_addr_type == AMDGV_FB_ADDR_VF_PHY) {
		translated_addr_in.vf_phy_addr.idx_vf = req_buff->vf_phy_addr.vf_idx;
		translated_addr_in.vf_phy_addr.addr = req_buff->vf_phy_addr.addr;

		ret = amdgv_gpumon_translate_fb_address(adev,
				AMDGV_GPUMON_FB_ADDR_VF_PHY,
				amdgv_fb_addr_type_to_gpumon_type(req_buff->dest_addr_type),
				translated_addr_in,
				&translated_addr_out);
	} else {
		return AMDGV_CMD__ERROR_INVALID_INPUT;
	}

	if (ret)
		return AMDGV_CMD__ERROR_GENERIC;

	if (req_buff->dest_addr_type == AMDGV_FB_ADDR_SOC_PHY) {
		rsp_buff->soc_phy_addr = translated_addr_out.soc_phy_addr;
	} else if (req_buff->dest_addr_type == AMDGV_FB_ADDR_BANK) {
		rsp_buff->bank_addr.stack_id = translated_addr_out.bank_addr.stack_id;
		rsp_buff->bank_addr.bank_group = translated_addr_out.bank_addr.bank_group;
		rsp_buff->bank_addr.bank = translated_addr_out.bank_addr.bank;
		rsp_buff->bank_addr.row = translated_addr_out.bank_addr.row;
		rsp_buff->bank_addr.column = translated_addr_out.bank_addr.column;
		rsp_buff->bank_addr.channel = translated_addr_out.bank_addr.channel;
		rsp_buff->bank_addr.subchannel = translated_addr_out.bank_addr.subchannel;
	} else if (req_buff->dest_addr_type == AMDGV_FB_ADDR_VF_PHY) {
		rsp_buff->vf_phy_addr.vf_idx = translated_addr_out.vf_phy_addr.idx_vf;
		rsp_buff->vf_phy_addr.addr = translated_addr_out.vf_phy_addr.addr;
	}

	cmd->output_size = sizeof(struct amdgv_cmd_translate_fb_address_rsp);

	return AMDGV_CMD__SUCCESS;
}

static uint8_t amdgv_reset_all_error_counts(amdgv_dev_t adev, struct amdgv_uni_cmd *cmd)
{
	if (cmd->input_size != sizeof(struct amdgv_cmd_dev_handle) ||
			cmd->version != AMDGV_CMD_VERSION_V1 || !adev)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	if (amdgv_gpumon_reset_all_error_counts(adev))
		return AMDGV_CMD__ERROR_GENERIC;

	cmd->output_size = 0;

	return AMDGV_CMD__SUCCESS;
}

static uint8_t amdgv_get_cper_records(amdgv_dev_t adev, struct amdgv_uni_cmd *cmd)
{
	struct amdgv_get_cper_records_input *input_data =
				(struct amdgv_get_cper_records_input *)cmd->input_buff_raw;
	struct amdgv_get_cper_records_output *output_data =
				(struct amdgv_get_cper_records_output *)cmd->output_buff_raw;
	uint8_t *buffer;
	int r;

	if (cmd->input_size != sizeof(struct amdgv_get_cper_records_input) ||
			cmd->version != AMDGV_CMD_VERSION_V1 || !adev)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	buffer = oss_alloc_memory(input_data->buf_size);
	if (!buffer) {
		return AMDGV_CMD__ERROR_GENERIC;
	}

	r = amdgv_gpumon_cper_get_entries(adev, input_data->rptr, buffer, input_data->buf_size,
					  &output_data->write_count, &output_data->overflow_count,
					  &output_data->left_size);
	if (r) {
		r = AMDGV_CMD__ERROR_GENERIC;
		goto out;
	}

	oss_copy_to_user(input_data->buf, buffer, input_data->buf_size);

	cmd->output_size = sizeof(struct amdgv_get_cper_records_output);

out:
	oss_free_memory(buffer);
	return r;
}

static uint8_t amdgv_load_ras_ta(amdgv_dev_t adev, struct amdgv_uni_cmd *cmd)
{
	struct amdgv_cmd_ras_ta_load_req *input_data =
			(struct amdgv_cmd_ras_ta_load_req *)cmd->input_buff_raw;
	struct amdgv_cmd_ras_ta_load_rsp *out_data =
			(struct amdgv_cmd_ras_ta_load_rsp *)cmd->output_buff_raw;

	struct amdgv_smi_cmd_ras_ta_load ras_ta_load = {0};
	uint32_t ta_version = 0, loaded_version = 0;
	enum amdgv_ras_ta_load_status ta_load_status;
	uint8_t *buf_ptr;

	if (cmd->input_size != sizeof(struct amdgv_cmd_ras_ta_load_req) ||
			cmd->version != AMDGV_CMD_VERSION_V1 || !adev)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	if (!input_data->version || !input_data->data_len || !input_data->data_addr) {
		return AMDGV_CMD__ERROR_GENERIC;
	}

	buf_ptr = oss_alloc_memory(input_data->data_len);
	if (!buf_ptr) {
		return AMDGV_CMD__ERROR_GENERIC;
	}

	oss_copy_from_user(buf_ptr, (uint8_t *)input_data->data_addr, input_data->data_len);

	if (amdgv_gpumon_ras_get_ta_version(adev, buf_ptr, &ta_version)) {
		goto load_err;
	}

	if (amdgv_gpumon_ras_get_loaded_ta_version(adev, &loaded_version))
		ta_load_status = AMDGV_RAS_TA_STATUS_LOADED;
	else  if (ta_version == loaded_version)
		ta_load_status = AMDGV_RAS_TA_STATUS_NO_CHANGE;
	else if (ta_version > loaded_version)
		ta_load_status = AMDGV_RAS_TA_STATUS_UPGRADED;
	else if (ta_version < loaded_version)
		ta_load_status = AMDGV_RAS_TA_STATUS_DOWNGRADED;
	else
		goto load_err;

	if (ta_load_status != AMDGV_RAS_TA_STATUS_NO_CHANGE) {
		ras_ta_load.version = input_data->version;
		ras_ta_load.in_data_len = input_data->data_len;
		ras_ta_load.in_data_addr = (uint64_t)buf_ptr;
		if (amdgv_gpumon_ras_ta_load(adev, &ras_ta_load)) {
			goto load_err;
		}
		out_data->ras_session_id = ras_ta_load.out_ras_session_id;
	} else {
		amdgv_gpumon_get_ras_session_id(adev, &(out_data->ras_session_id));
	}

	out_data->ta_status = ta_load_status;
	oss_free_memory(buf_ptr);

	cmd->output_size = sizeof(struct amdgv_cmd_ras_ta_load_rsp);

	return AMDGV_CMD__SUCCESS;

load_err:
	oss_free_memory(buf_ptr);
	return AMDGV_CMD__ERROR_GENERIC;
}

static uint8_t amdgv_unload_ras_ta(amdgv_dev_t adev, struct amdgv_uni_cmd *cmd)
{
	struct amdgv_cmd_ras_ta_unload_req *input_data =
			(struct amdgv_cmd_ras_ta_unload_req *)cmd->input_buff_raw;
	struct amdgv_smi_cmd_ras_ta_unload ras_ta_unload = {0};

	if (cmd->input_size != sizeof(struct amdgv_cmd_ras_ta_unload_req) ||
			cmd->version != AMDGV_CMD_VERSION_V1 || !adev)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	ras_ta_unload.ras_session_id = input_data->ras_session_id;

	if (amdgv_gpumon_ras_ta_unload(adev, &ras_ta_unload))
		return AMDGV_CMD__ERROR_GENERIC;

	return AMDGV_CMD__SUCCESS;
}

static uint8_t amdgv_ras_ecc_inject(amdgv_dev_t adev, struct amdgv_uni_cmd *cmd)
{
	struct amdgv_cmd_ras_inject_error *input_data =
			(struct amdgv_cmd_ras_inject_error *)cmd->input_buff_raw;
	struct amdgv_smi_ras_error_inject_info inject_info = {0};

	if (cmd->input_size != sizeof(struct amdgv_cmd_ras_inject_error) ||
			cmd->version != AMDGV_CMD_VERSION_V1 || !adev)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	inject_info.block_id = amdgv_block_to_ta_block(input_data->device_info.block_id);
	inject_info.sub_block_index = input_data->device_info.subblock_id;
	inject_info.address = input_data->address;
	inject_info.inject_error_type = amdgv_ras_error_type_to_ta_error_type(input_data->error_type);
	inject_info.method = input_data->method;
	inject_info.vf_idx = input_data->vf_idx;
	inject_info.mask = input_data->chiplet;

	if (amdgv_gpumon_ras_error_inject(adev, &inject_info))
		return AMDGV_CMD__ERROR_GENERIC;

	cmd->output_size = 0;

	return AMDGV_CMD__SUCCESS;
}

static uint8_t amdgv_get_block_ecc_info(amdgv_dev_t adev, struct amdgv_uni_cmd *cmd)
{
	struct amdgv_cmd_dev_block_info *input_data =
			(struct amdgv_cmd_dev_block_info *)cmd->input_buff_raw;
	struct amdgv_cmd_ecc_count *output_data =
			(struct amdgv_cmd_ecc_count *)cmd->output_buff_raw;
	struct amdgv_smi_ras_query_if query_info = {0};

	if (cmd->input_size != sizeof(struct amdgv_cmd_dev_block_info) ||
			cmd->version != AMDGV_CMD_VERSION_V1 || !adev)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	query_info.head.block = input_data->block_id;
	query_info.head.sub_block_index = input_data->subblock_id;

	if (amdgv_gpumon_get_ecc_info(adev, &query_info))
		return AMDGV_CMD__ERROR_GENERIC;

	output_data->corr_error_cnt = query_info.ce_count;
	output_data->uncorr_error_cnt = query_info.ue_count;
	output_data->deferred_error_cnt = query_info.de_count;

	cmd->output_size = sizeof(struct amdgv_cmd_ecc_count);
	return AMDGV_CMD__SUCCESS;
}

static uint8_t amdgv_get_bad_pages(amdgv_dev_t adev, struct amdgv_uni_cmd *cmd)
{
	struct amdgv_cmd_req_bad_pages_group *input_data =
			(struct amdgv_cmd_req_bad_pages_group *)cmd->input_buff_raw;
	struct amdgv_cmd_bad_pages_info *output_data =
			(struct amdgv_cmd_bad_pages_info *)cmd->output_buff_raw;
	uint32_t ret;

	if (cmd->input_size != sizeof(struct amdgv_cmd_req_bad_pages_group) ||
			cmd->version != AMDGV_CMD_VERSION_V1 || !adev)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	ret = __amdgv_get_bad_pages(adev, input_data->group_index, output_data);
	if (ret)
		return AMDGV_CMD__ERROR_GENERIC;

	cmd->output_size = sizeof(struct amdgv_cmd_bad_pages_info);
	return AMDGV_CMD__SUCCESS;
}

static uint8_t amdgv_clear_bad_page_info(amdgv_dev_t adev, struct amdgv_uni_cmd *cmd)
{
	if (cmd->input_size != sizeof(struct amdgv_cmd_dev_handle) ||
			cmd->version != AMDGV_CMD_VERSION_V1)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	if (amdgv_gpumon_ras_eeprom_clear(adev))
		return AMDGV_CMD__ERROR_GENERIC;

	return AMDGV_CMD__SUCCESS;
}

static amdgv_cmd_func_map amdgv_ras_func[] = {
	{AMDGV_CMD_GET_BLOCK_ECC_STATUS, amdgv_get_block_ecc_info},
	{AMDGV_CMD_RAS_INJECT_ERROR, amdgv_ras_ecc_inject},
	{AMDGV_CMD_GET_BAD_PAGES, amdgv_get_bad_pages},
	{AMDGV_CMD_CLEAR_BAD_PAGE_INFO, amdgv_clear_bad_page_info},
	{AMDGV_CMD_RAS_TA_LOAD, amdgv_load_ras_ta},
	{AMDGV_CMD_RAS_TA_UNLOAD, amdgv_unload_ras_ta},
	{AMDGV_CMD_RAS_GET_SAFE_FB_ADDRESS_RANGES, amdgv_get_safe_fb_addr_ranges},
	{AMDGV_CMD_TRANSLATE_FB_ADDRESS, amdgv_translate_fb_address},
	{AMDGV_CMD_RAS_RESET_ALL_ERROR_COUNTS, amdgv_reset_all_error_counts},
	{AMDGV_CMD_GET_CPER_RECORDS, amdgv_get_cper_records}
};

uint8_t amdgv_handle_uni_cmd(void *data, struct amdgv_uni_cmd *cmd)
{
	static amdgv_cmd_func_map *func = amdgv_ras_func;
	int i;

	for (i = 0; i < ARRAY_SIZE(amdgv_ras_func); i++)
		if (func[i].cmd_id == cmd->cmd_id)
			return func[i].func((amdgv_dev_t *)data, cmd);

	return AMDGV_CMD__ERROR_UKNOWN_CMD;
}
