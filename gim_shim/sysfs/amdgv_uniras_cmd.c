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

#include <linux/fs.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/sort.h>
#include <linux/version.h>
#include <linux/slab.h>
#include <linux/mm.h>

#include "amdgv_uniras_cmd.h"
#include "amdgv_cmd_uni_def.h"
#include "amdgv_asic.h"
#include "amdgv_api.h"
#include "amdgv_gpumon.h"
#include "gim_debug.h"
#include "gim.h"

extern struct list_head gim_device_list;
extern struct mutex gim_device_list_lock;
static struct amdgv_uni_cmd amdgv_uni_cmd;

typedef uint8_t (*cmd_func)(struct amdgv_uni_cmd *cmd);

typedef struct amdgv_cmd_func_map {
	uint32_t cmd_id;
	cmd_func func;
} amdgv_cmd_func_map;

static enum amdgv_cmd_asic_type amd_asic_type_to_amdgv_cmd_asic_type(enum amd_asic_type asic_type)
{
	switch (asic_type) {
	case CHIP_MI200:
		return AMDGV_CMD_CHIP_MI200;
	case CHIP_NAVI32:
		return AMDGV_CMD_CHIP_NAVI32;
	case CHIP_MI300X:
		return AMDGV_CMD_CHIP_MI300X;
	case CHIP_MI308X:
		return AMDGV_CMD_CHIP_MI308X;
	case CHIP_MI350X:
		return AMDGV_CMD_CHIP_MI350X;
	case CHIP_LAST:
		return AMDGV_CMD_CHIP_LAST;
	default:
		return AMDGV_CMD_CHIP_UNKNOWN;
	}
}

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

static uint8_t amdgv_handle_ras_cmd(struct amdgv_uni_cmd *cmd);
long amdgv_uni_cmd_handler(void *arg)
{
	if (copy_from_user(&amdgv_uni_cmd, arg, sizeof(struct amdgv_uni_cmd)))
		return -EFAULT;

	amdgv_uni_cmd.cmd_res = AMDGV_CMD__ERROR_INVALID_INPUT;
	amdgv_uni_cmd.output_size = 0;

	if (amdgv_uni_cmd.cmd_id == AMDGV_CMD_QUERY_INTERFACE_VERSION &&
		amdgv_uni_cmd.version == AMDGV_CMD_VERSION_V1 &&
		amdgv_uni_cmd.input_size == sizeof(struct amdgv_query_interface_version_req)) {
			struct amdgv_query_interface_version_rsp *ver_rsp =
					(struct amdgv_query_interface_version_rsp *)amdgv_uni_cmd.output_buff_raw;
			amdgv_uni_cmd.output_size = sizeof(struct amdgv_query_interface_version_rsp);

			ver_rsp->major_ver = AMDGV_INTERFACE_MAJOR_VERSION;
			ver_rsp->minor_ver = AMDGV_INTERFACE_MINOR_VERSION;

			amdgv_uni_cmd.output_size = sizeof(struct amdgv_query_interface_version_rsp);
			amdgv_uni_cmd.cmd_res = AMDGV_CMD__SUCCESS;
	} else {
			amdgv_uni_cmd.cmd_res = amdgv_handle_ras_cmd(&amdgv_uni_cmd);
	}

	if (copy_to_user(arg, &amdgv_uni_cmd, sizeof(struct amdgv_uni_cmd)))
		return -EFAULT;

	// Clear command buffer
	memset(&amdgv_uni_cmd, 0, sizeof(struct amdgv_uni_cmd));

	return 0;
}

static void *gim_get_dev_u64(uint64_t dev_handle)
{
	amdgv_dev_t adev = NULL;
	struct gim_dev_data *dev_data = NULL;
	uint64_t dev_uuid;

	mutex_lock(&gim_device_list_lock);
	list_for_each_entry(dev_data, &gim_device_list, list) {
		if (amdgv_gpumon_get_dev_uuid(dev_data->adev, &dev_uuid)) {
			adev = NULL;
			continue;
		}
		if (dev_handle == dev_uuid) {
			adev = dev_data->adev;
			break;
		}
	}
	mutex_unlock(&gim_device_list_lock);

	if (adev == NULL)
		gim_warn("Failed to get device handler\n");
	return adev;
}

static void __amdgv_get_device_info_v1(struct gim_dev_data *dev_data, struct amdgv_cmd_dev_info *amdgv_dev)
{
	union amdgv_dev_info dev_info;
	uint32_t asic_type;

	if (!amdgv_gpumon_get_asic_type(dev_data->adev, &asic_type))
		amdgv_dev->asic_type = amd_asic_type_to_amdgv_cmd_asic_type(asic_type);
	else
		amdgv_dev->asic_type = AMDGV_CMD_CHIP_UNKNOWN;

	if (!amdgv_get_dev_info(dev_data->adev, AMDGV_GET_ENABLED_VF_NUM, &dev_info))
		amdgv_dev->vf_num = dev_info.vf.num_enabled_vf;
	else
		amdgv_dev->vf_num =  0;

	amdgv_dev->bdf = dev_data->init_data.info.bdf;

	if (amdgv_gpumon_get_dev_uuid(dev_data->adev, &amdgv_dev->dev_handle))
		amdgv_dev->dev_handle = 0;

	if (amdgv_gpumon_get_ecc_support_flag(dev_data->adev, &amdgv_dev->ecc_enabled,
			&amdgv_dev->ecc_supported)) {
				amdgv_dev->ecc_enabled = 0;
				amdgv_dev->ecc_supported = 0;
	}
}

static void __amdgv_get_device_info_v2(struct gim_dev_data *dev_data, struct amdgv_cmd_dev_info *amdgv_dev,
					struct amdgv_cmd_dev_info_ex *amdgv_dev_ex)
{
	union amdgv_dev_info dev_info;
	uint32_t asic_type;

	if (!amdgv_gpumon_get_asic_type(dev_data->adev, &asic_type))
		amdgv_dev->asic_type = amd_asic_type_to_amdgv_cmd_asic_type(asic_type);
	else
		amdgv_dev->asic_type = AMDGV_CMD_CHIP_UNKNOWN;

	if (!amdgv_get_dev_info(dev_data->adev, AMDGV_GET_ENABLED_VF_NUM, &dev_info))
		amdgv_dev->vf_num = dev_info.vf.num_enabled_vf;
	else
		amdgv_dev->vf_num =  0;

	amdgv_dev->bdf = dev_data->init_data.info.bdf;

	if (amdgv_gpumon_get_dev_uuid(dev_data->adev, &amdgv_dev->dev_handle))
		amdgv_dev->dev_handle = 0;

	if (amdgv_gpumon_get_ecc_support_flag(dev_data->adev, &amdgv_dev->ecc_enabled,
			&amdgv_dev->ecc_supported)) {
				amdgv_dev->ecc_enabled = 0;
				amdgv_dev->ecc_supported = 0;
	}

	if (!amdgv_get_dev_info(dev_data->adev, AMDGV_GET_OAM_IDX, &dev_info))
		amdgv_dev_ex->oam_id = dev_info.oam.oam_idx;
}

static uint8_t  amdgv_get_device_info(struct amdgv_uni_cmd *cmd)
{
	struct amdgv_cmd_devices_info *output_data =
		(struct amdgv_cmd_devices_info *)cmd->output_buff_raw;
	struct gim_dev_data *dev_data;
	uint8_t i = 0;

	if (cmd->version != AMDGV_CMD_VERSION_V1 && cmd->version != AMDGV_CMD_VERSION_V2)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	mutex_lock(&gim_device_list_lock);
	list_for_each_entry(dev_data, &gim_device_list, list) {
		if (cmd->version == AMDGV_CMD_VERSION_V1)
			__amdgv_get_device_info_v1(dev_data, &output_data->devs[i]);
		else
			__amdgv_get_device_info_v2(dev_data, &output_data->devs[i], &output_data->devs_ex[i]);
		i++;
	}
	mutex_unlock(&gim_device_list_lock);

	output_data->dev_num = i;

	if (cmd->version == AMDGV_CMD_VERSION_V1)
		cmd->output_size = sizeof(struct amdgv_cmd_devices_info) -
			AMDGV_CMD_MAX_GPU_NUM * sizeof(struct amdgv_cmd_dev_info_ex);
	else
		cmd->output_size = sizeof(struct amdgv_cmd_devices_info);

	return AMDGV_CMD__SUCCESS;
}

static uint8_t amdgv_get_safe_fb_addr_ranges(struct amdgv_uni_cmd *cmd)
{
	struct amdgv_cmd_dev_handle *req_buff = (struct amdgv_cmd_dev_handle *)cmd->input_buff_raw;
	struct amdgv_cmd_ras_safe_fb_address_ranges_rsp *rsp_buff =
			(struct amdgv_cmd_ras_safe_fb_address_ranges_rsp *)cmd->output_buff_raw;
	struct amdgv_gpumon_ras_safe_fb_address_ranges *libgv_safe_ranges;
	amdgv_dev_t adev;
	int ret, i;

	if (cmd->input_size != sizeof(struct amdgv_cmd_dev_handle) ||
			cmd->version != AMDGV_CMD_VERSION_V1)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	adev = gim_get_dev_u64(req_buff->dev_handle);
	if (!adev) {
		gim_warn("Failed to get device handle!\n");
		return AMDGV_CMD__ERROR_GENERIC;
	}

	libgv_safe_ranges = gim_oss_interfaces.alloc_memory(sizeof(*libgv_safe_ranges));
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

	gim_oss_interfaces.free_memory(libgv_safe_ranges);
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

static uint8_t amdgv_translate_fb_address(struct amdgv_uni_cmd *cmd)
{
	struct amdgv_cmd_translate_fb_address_req *req_buff =
			(struct amdgv_cmd_translate_fb_address_req *)cmd->input_buff_raw;
	struct amdgv_cmd_translate_fb_address_rsp *rsp_buff =
			(struct amdgv_cmd_translate_fb_address_rsp *)cmd->output_buff_raw;
	union amdgv_gpumon_translate_fb_address translated_addr_in = { 0 };
	union amdgv_gpumon_translate_fb_address translated_addr_out = { 0 };
	amdgv_dev_t adev;
	int ret = AMDGV_CMD__ERROR_GENERIC;

	if (cmd->input_size != sizeof(struct amdgv_cmd_translate_fb_address_req) ||
			cmd->version != AMDGV_CMD_VERSION_V1)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	adev = gim_get_dev_u64(req_buff->dev.dev_handle);
	if (!adev) {
		gim_warn("Failed to get device handle when loading ras ta!\n");
		return AMDGV_CMD__ERROR_GENERIC;
	}

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

static uint8_t amdgv_reset_all_error_counts(struct amdgv_uni_cmd *cmd)
{
	struct amdgv_cmd_dev_handle *input_data =
			(struct amdgv_cmd_dev_handle *)cmd->input_buff_raw;
	amdgv_dev_t *adev;

	if (cmd->input_size != sizeof(struct amdgv_cmd_dev_handle) ||
			cmd->version != AMDGV_CMD_VERSION_V1)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	adev = gim_get_dev_u64(input_data->dev_handle);
	if (!adev) {
		gim_warn("Failed to get device handle when loading ras ta!\n");
		return AMDGV_CMD__ERROR_GENERIC;
	}

	if (amdgv_gpumon_reset_all_error_counts(adev))
		return AMDGV_CMD__ERROR_GENERIC;

	cmd->output_size = 0;

	return AMDGV_CMD__SUCCESS;
}

static uint8_t amdgv_get_link_topology(struct amdgv_uni_cmd *cmd)
{
	struct amdgv_dev_pair_info *input_data =
			(struct amdgv_dev_pair_info *) cmd->input_buff_raw;
	struct amdgv_dev_link_topology *output_data =
			(struct amdgv_dev_link_topology *)cmd->output_buff_raw;
	struct amdgv_gpumon_link_topology_info link_topology;
	amdgv_dev_t src_adev, dst_adev;
	int ret;

	if (cmd->input_size != sizeof(struct amdgv_dev_pair_info) ||
			cmd->version != AMDGV_CMD_VERSION_V1)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	src_adev = gim_get_dev_u64(input_data->src.dev_handle);
	if (!src_adev)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	dst_adev = gim_get_dev_u64(input_data->dst.dev_handle);
	if (!dst_adev)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	ret = amdgv_gpumon_get_link_topology(src_adev, dst_adev, &link_topology);
	if (ret)
		return AMDGV_CMD__ERROR_GENERIC;

	output_data->weight = link_topology.weight;
	output_data->link_status = link_topology.link_status;
	output_data->link_type = link_topology.link_type;
	output_data->num_hops = link_topology.num_hops;
	output_data->fb_sharing = link_topology.is_fb_sharing_enabled;

	cmd->output_size = sizeof(struct amdgv_dev_link_topology);
	return AMDGV_CMD__SUCCESS;
}

static int amdgv_copy_cper_records_to_user(struct amdgv_get_cper_records_input *input_data,
					   uint8_t *buffer)
{
	uint8_t *buf;
	long num_pages = 0;
	unsigned int gup_flags;
	struct mm_struct *mm;
	unsigned long nr_pages;
	struct page **pages;
	uint32_t i = 0;

	mm = current->mm; // Get memory management structure

	nr_pages = (input_data->buf_size + PAGE_SIZE - 1) >> PAGE_SHIFT;

	pages = kmalloc(nr_pages * sizeof(struct page *), GFP_KERNEL);
	if (!pages) {
		return -ENOMEM;
	}

#if defined(HAVE_UP_DOWN_READ_MMAP_LOCK_ARG)
	down_read(&current->mm->mmap_lock);
#else
	down_read(&current->mm->mmap_sem);
#endif
	gup_flags = FOLL_WRITE;

#if defined(HAVE_GET_USER_PAGES_REMOTE_6_ARG)
	num_pages = get_user_pages_remote(mm, (unsigned long)input_data->buf,
					  nr_pages, gup_flags, pages, NULL);
#elif defined(HAVE_GET_USER_PAGES_REMOTE_7_ARG)
	num_pages = get_user_pages_remote(mm, (unsigned long)input_data->buf,
					  nr_pages, gup_flags, pages, NULL,
					  NULL);
#else
	num_pages = get_user_pages_remote(NULL, mm, (unsigned long)input_data->buf,
					  nr_pages, gup_flags, pages, NULL, NULL);
#endif
	if (num_pages <= 0) {
		kfree(pages);
#if defined(HAVE_UP_DOWN_READ_MMAP_LOCK_ARG)
		up_read(&current->mm->mmap_lock);
#else
		up_read(&current->mm->mmap_sem);
#endif
		return AMDGV_CMD__ERROR_GENERIC;
	}
	// Access mapped metrics table in kernel space
	// map array of pages into virtual contiguous memory
	buf = (uint8_t *)vmap(pages, num_pages, VM_MAP, PAGE_KERNEL);

	memcpy(buf, buffer, input_data->buf_size);

	// Unmap and release mapped pages and
	// free the virtual contiguous memory
	vunmap(buf);
	for (i = 0; i < num_pages; i++) {
		if (!PageReserved(pages[i]))
			SetPageDirty(pages[i]);
		put_page(pages[i]);
	}

	kfree(pages);
#if defined(HAVE_UP_DOWN_READ_MMAP_LOCK_ARG)
	up_read(&current->mm->mmap_lock);
#else
	up_read(&current->mm->mmap_sem);
#endif

	return AMDGV_CMD__SUCCESS;
}

static uint8_t amdgv_get_cper_records(struct amdgv_uni_cmd *cmd)
{
	struct amdgv_get_cper_records_input *input_data =
				(struct amdgv_get_cper_records_input *)cmd->input_buff_raw;
	struct amdgv_get_cper_records_output *output_data =
				(struct amdgv_get_cper_records_output *)cmd->output_buff_raw;
	amdgv_dev_t *adev;
	uint8_t *buffer;
	int r;

	if (cmd->input_size != sizeof(struct amdgv_get_cper_records_input) ||
			cmd->version != AMDGV_CMD_VERSION_V1)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	buffer = kcalloc(input_data->buf_size, 1, GFP_KERNEL);
	if (!buffer) {
		gim_warn("Failed to alloc memory, size = %lld\n", input_data->buf_size);
		return AMDGV_CMD__ERROR_GENERIC;
	}

	adev = gim_get_dev_u64(input_data->dev_handle);

	r = amdgv_gpumon_cper_get_entries(adev, input_data->rptr, buffer, input_data->buf_size,
					  &output_data->write_count, &output_data->overflow_count,
					  &output_data->left_size);
	if (r) {
		gim_warn("Failed to get CPER entries, r = %d\n", r);
		r = AMDGV_CMD__ERROR_GENERIC;
		goto out;
	}

	r = amdgv_copy_cper_records_to_user(input_data, buffer);
	if (r) {
		gim_warn("Failed to copy CPER records to user, r = %d\n", r);
		r = AMDGV_CMD__ERROR_GENERIC;
		goto out;
	}

	cmd->output_size = sizeof(struct amdgv_get_cper_records_output);

out:
	kfree(buffer);
	return r;
}

static uint8_t amdgv_load_ras_ta(struct amdgv_uni_cmd *cmd)
{
	struct amdgv_cmd_ras_ta_load_req *input_data =
			(struct amdgv_cmd_ras_ta_load_req *)cmd->input_buff_raw;
	struct amdgv_cmd_ras_ta_load_rsp *out_data =
			(struct amdgv_cmd_ras_ta_load_rsp *)cmd->output_buff_raw;

	struct amdgv_smi_cmd_ras_ta_load ras_ta_load = {0};
	amdgv_dev_t adev = NULL;
	uint32_t ta_version = 0, loaded_version = 0;
	enum amdgv_ras_ta_load_status ta_load_status;
	uint8_t *buf_ptr;

	if (cmd->input_size != sizeof(struct amdgv_cmd_ras_ta_load_req) ||
			cmd->version != AMDGV_CMD_VERSION_V1)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	if (!input_data->version || !input_data->data_len || !input_data->data_addr) {
		gim_warn("Invaild ras ta parameter: version:0x%x, data_len:0x%x, data_addr:0x%llx\n",
			input_data->version, input_data->data_len, input_data->data_addr);
		return AMDGV_CMD__ERROR_GENERIC;
	}
	adev = gim_get_dev_u64(input_data->dev.dev_handle);
	if (!adev) {
		gim_warn("Failed to get device handle when loading ras ta!\n");
		return AMDGV_CMD__ERROR_GENERIC;
	}

	buf_ptr = gim_oss_interfaces.alloc_memory(input_data->data_len);
	if (!buf_ptr) {
		gim_warn("Failed to alloc memory!\n");
		return AMDGV_CMD__ERROR_GENERIC;
	}

	if (copy_from_user(buf_ptr, (uint8_t *)input_data->data_addr, input_data->data_len)) {
		gim_warn("Failed to copy data from user!\n");
		goto load_err;
	}

	if (amdgv_gpumon_ras_get_ta_version(adev, buf_ptr, &ta_version)) {
		gim_warn("Failed to get ras ta version!\n");
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
			gim_warn("Failed to load RAS TA!\n");
			goto load_err;
		}
		out_data->ras_session_id = ras_ta_load.out_ras_session_id;
	} else {
		amdgv_gpumon_get_ras_session_id(adev, &(out_data->ras_session_id));
	}

	out_data->ta_status = ta_load_status;
	gim_oss_interfaces.free_memory(buf_ptr);

	cmd->output_size = sizeof(struct amdgv_cmd_ras_ta_load_rsp);

	return AMDGV_CMD__SUCCESS;

load_err:
	gim_oss_interfaces.free_memory(buf_ptr);
	return AMDGV_CMD__ERROR_GENERIC;
}

static uint8_t amdgv_unload_ras_ta(struct amdgv_uni_cmd *cmd)
{
	struct amdgv_cmd_ras_ta_unload_req *input_data =
			(struct amdgv_cmd_ras_ta_unload_req *)cmd->input_buff_raw;
	struct amdgv_smi_cmd_ras_ta_unload ras_ta_unload = {0};
	amdgv_dev_t adev = NULL;

	if (cmd->input_size != sizeof(struct amdgv_cmd_ras_ta_unload_req) ||
			cmd->version != AMDGV_CMD_VERSION_V1)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	adev = gim_get_dev_u64(input_data->dev.dev_handle);
	if (!adev) {
		gim_warn("Failed to get device handle when unloading ras ta!\n");
		return AMDGV_CMD__ERROR_GENERIC;
	}

	ras_ta_unload.ras_session_id = input_data->ras_session_id;

	if (amdgv_gpumon_ras_ta_unload(adev, &ras_ta_unload))
		return AMDGV_CMD__ERROR_GENERIC;

	return AMDGV_CMD__SUCCESS;
}

static uint8_t amdgv_ras_ecc_inject(struct amdgv_uni_cmd *cmd)
{
	struct amdgv_cmd_ras_inject_error *input_data =
			(struct amdgv_cmd_ras_inject_error *)cmd->input_buff_raw;
	struct amdgv_smi_ras_error_inject_info inject_info = {0};
	amdgv_dev_t adev = NULL;

	if (cmd->input_size != sizeof(struct amdgv_cmd_ras_inject_error) ||
			cmd->version != AMDGV_CMD_VERSION_V1)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	inject_info.block_id = amdgv_block_to_ta_block(input_data->device_info.block_id);
	inject_info.sub_block_index = input_data->device_info.subblock_id;
	inject_info.address = input_data->address;
	inject_info.inject_error_type = amdgv_ras_error_type_to_ta_error_type(input_data->error_type);
	inject_info.method = input_data->method;
	inject_info.vf_idx = input_data->vf_idx;
	inject_info.mask = input_data->chiplet;

	adev = gim_get_dev_u64(input_data->device_info.dev.dev_handle);
	if (amdgv_gpumon_ras_error_inject(adev, &inject_info))
		return AMDGV_CMD__ERROR_GENERIC;

	cmd->output_size = 0;

	return AMDGV_CMD__SUCCESS;
}

static uint8_t amdgv_get_block_ecc_info(struct amdgv_uni_cmd *cmd)
{
	struct amdgv_cmd_dev_block_info *input_data =
			(struct amdgv_cmd_dev_block_info *)cmd->input_buff_raw;
	struct amdgv_cmd_ecc_count *output_data =
			(struct amdgv_cmd_ecc_count *)cmd->output_buff_raw;
	struct amdgv_smi_ras_query_if query_info = {0};
	amdgv_dev_t adev = NULL;

	if (cmd->input_size != sizeof(struct amdgv_cmd_dev_block_info) ||
			cmd->version != AMDGV_CMD_VERSION_V1)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	query_info.head.block = input_data->block_id;
	query_info.head.sub_block_index = input_data->subblock_id;

	adev = gim_get_dev_u64(input_data->dev.dev_handle);
	if (amdgv_gpumon_get_ecc_info(adev, &query_info))
		return AMDGV_CMD__ERROR_GENERIC;

	output_data->corr_error_cnt = query_info.ce_count;
	output_data->uncorr_error_cnt = query_info.ue_count;
	output_data->deferred_error_cnt = query_info.de_count;

	cmd->output_size = sizeof(struct amdgv_cmd_ecc_count);
	return AMDGV_CMD__SUCCESS;
}

static uint8_t amdgv_get_bad_pages(struct amdgv_uni_cmd *cmd)
{
	struct amdgv_cmd_req_bad_pages_group *input_data =
			(struct amdgv_cmd_req_bad_pages_group *)cmd->input_buff_raw;
	struct amdgv_cmd_bad_pages_info *output_data =
			(struct amdgv_cmd_bad_pages_info *)cmd->output_buff_raw;
	amdgv_dev_t *adev;
	uint32_t ret;

	if (cmd->input_size != sizeof(struct amdgv_cmd_req_bad_pages_group) ||
			cmd->version != AMDGV_CMD_VERSION_V1)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	adev = gim_get_dev_u64(input_data->device.dev_handle);
	if (!adev)
		return AMDGV_CMD__ERROR_GENERIC;

	ret = __amdgv_get_bad_pages(adev, input_data->group_index, output_data);
	if (ret)
		return AMDGV_CMD__ERROR_GENERIC;

	cmd->output_size = sizeof(struct amdgv_cmd_bad_pages_info);
	return AMDGV_CMD__SUCCESS;
}

static uint8_t amdgv_clear_bad_page_info(struct amdgv_uni_cmd *cmd)
{
	struct amdgv_cmd_dev_handle *input_data =
			(struct amdgv_cmd_dev_handle *)cmd->input_buff_raw;
	amdgv_dev_t *adev;

	if (cmd->input_size != sizeof(struct amdgv_cmd_dev_handle) ||
			cmd->version != AMDGV_CMD_VERSION_V1)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	adev = gim_get_dev_u64(input_data->dev_handle);

	if (amdgv_gpumon_ras_eeprom_clear(adev))
		return AMDGV_CMD__ERROR_GENERIC;

	return AMDGV_CMD__SUCCESS;
}

static amdgv_cmd_func_map amdgv_ras_func[] = {
	{AMDGV_CMD_GET_DEVICES_INFO, amdgv_get_device_info},
	{AMDGV_CMD_GET_BLOCK_ECC_STATUS, amdgv_get_block_ecc_info},
	{AMDGV_CMD_RAS_INJECT_ERROR, amdgv_ras_ecc_inject},
	{AMDGV_CMD_GET_BAD_PAGES, amdgv_get_bad_pages},
	{AMDGV_CMD_CLEAR_BAD_PAGE_INFO, amdgv_clear_bad_page_info},
	{AMDGV_CMD_RAS_TA_LOAD, amdgv_load_ras_ta},
	{AMDGV_CMD_RAS_TA_UNLOAD, amdgv_unload_ras_ta},
	{AMDGV_CMD_RAS_GET_SAFE_FB_ADDRESS_RANGES, amdgv_get_safe_fb_addr_ranges},
	{AMDGV_CMD_TRANSLATE_FB_ADDRESS, amdgv_translate_fb_address},
	{AMDGV_CMD_RAS_RESET_ALL_ERROR_COUNTS, amdgv_reset_all_error_counts},
	{AMDGV_CMD_GET_LINK_TOPOLOGY, amdgv_get_link_topology},
	{AMDGV_CMD_GET_CPER_RECORDS, amdgv_get_cper_records}
};

static uint8_t amdgv_handle_ras_cmd(struct amdgv_uni_cmd *cmd)
{
	static amdgv_cmd_func_map *func = amdgv_ras_func;
	int i;

	for (i = 0; i < ARRAY_SIZE(amdgv_ras_func); i++)
		if (func[i].cmd_id == cmd->cmd_id)
			return func[i].func(cmd);

	return AMDGV_CMD__ERROR_UKNOWN_CMD;
}
