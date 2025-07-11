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

#define AMDGV_UNI_CMD _IOWR('R', 0, struct amdgv_uni_cmd)

bool amdgv_is_uni_cmd(unsigned int cmd)
{
	if (cmd == AMDGV_UNI_CMD)
		return true;
	else
		return false;
}

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

long amdgv_uni_cmd_handler(void *arg)
{
	amdgv_dev_t adev = NULL;
	struct amdgv_uni_cmd *amdgv_uni_cmd;

	amdgv_uni_cmd = gim_vmalloc(sizeof(struct amdgv_uni_cmd));
	if (!amdgv_uni_cmd) {
		gim_warn("Cannot allocate memory for uni command\n");
		return -ENOMEM;
	}

	if (copy_from_user(amdgv_uni_cmd, arg, sizeof(struct amdgv_uni_cmd))) {
		gim_vfree(amdgv_uni_cmd);
		return -EFAULT;
	}

	amdgv_uni_cmd->cmd_res = AMDGV_CMD__ERROR_INVALID_INPUT;
	amdgv_uni_cmd->output_size = 0;

	if (amdgv_uni_cmd->cmd_id == AMDGV_CMD_QUERY_INTERFACE_VERSION &&
		amdgv_uni_cmd->version == AMDGV_CMD_VERSION_V1 &&
		amdgv_uni_cmd->input_size == sizeof(struct amdgv_query_interface_version_req)) {
			struct amdgv_query_interface_version_rsp *ver_rsp =
					(struct amdgv_query_interface_version_rsp *)amdgv_uni_cmd->output_buff_raw;
			amdgv_uni_cmd->output_size = sizeof(struct amdgv_query_interface_version_rsp);

			ver_rsp->major_ver = AMDGV_INTERFACE_MAJOR_VERSION;
			ver_rsp->minor_ver = AMDGV_INTERFACE_MINOR_VERSION;

			amdgv_uni_cmd->output_size = sizeof(struct amdgv_query_interface_version_rsp);
			amdgv_uni_cmd->cmd_res = AMDGV_CMD__SUCCESS;
	} else if (amdgv_uni_cmd->cmd_id == AMDGV_CMD_GET_DEVICES_INFO) {
			amdgv_uni_cmd->cmd_res = amdgv_get_device_info(amdgv_uni_cmd);
	} else {
			struct amdgv_cmd_dev_handle *input_data  = (struct amdgv_cmd_dev_handle *)amdgv_uni_cmd->input_buff_raw;
			adev = gim_get_dev_u64(input_data->dev_handle);
			amdgv_uni_cmd->cmd_res = amdgv_handle_uni_cmd(adev, amdgv_uni_cmd);
	}

	if (copy_to_user(arg, amdgv_uni_cmd, sizeof(struct amdgv_uni_cmd))) {
		gim_vfree(amdgv_uni_cmd);
		return -EFAULT;
	}

	// Clear command buffer
	memset(amdgv_uni_cmd, 0, sizeof(struct amdgv_uni_cmd));
	gim_vfree(amdgv_uni_cmd);

	return 0;
}

