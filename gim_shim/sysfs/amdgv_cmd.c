/*
 * Copyright (c) 2018-2019 Advanced Micro Devices, Inc. All rights reserved.
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
#include <linux/version.h>

#include "gim.h"
#include "gim_debug.h"
#include "gim_monitor.h"
#include "amdgv_cmd.h"

#define AMDGV_CMD_MINOR_COUNT 1
#define AMDGV_CMD _IOWR('R', 0, struct amdgv_cmd)

static long amdgv_ioctl_handler(struct file *, unsigned int cmd, unsigned long arg);

static dev_t amdgv_dev;
static struct class *amdgv_class;
static struct cdev amdgv_cdev;

static struct amdgv_cmd *amdgv_cmd;

static void *gim_get_dev(uint32_t dev_handle)
{
	amdgv_dev_t adev = NULL;
	struct gim_dev_data *dev_data = NULL;

	mutex_lock(&gim_device_list_lock);
	list_for_each_entry(dev_data, &gim_device_list, list) {
		if (dev_handle == hash_64(dev_data->init_data.info.dev_id ^ dev_data->init_data.info.bdf, 64)) {
			adev = dev_data->adev;
			break;
		}
	}
	mutex_unlock(&gim_device_list_lock);

	if (adev == NULL)
		gim_warn("Failed to get device handler\n");
	return adev;
}

static struct gim_dev_data *gim_get_dev_data(void *adev)
{
	struct gim_dev_data *dev_data = NULL;

	mutex_lock(&gim_device_list_lock);
	list_for_each_entry(dev_data, &gim_device_list, list) {
		if (adev == dev_data->adev)
			break;
	}
	mutex_unlock(&gim_device_list_lock);

	if (!dev_data || dev_data->adev != adev) {
		gim_warn("Failed to get device data\n");
		return NULL;
	}
	return dev_data;
}

static enum amdgv_cmd_asic_type amd_asic_type_to_amdgv_cmd_asic_type(enum amd_asic_type asic_type)
{
	switch (asic_type) {
	case CHIP_MI300X:
		return AMDGV_CMD_CHIP_MI300X;
	case CHIP_MI308X:
		return AMDGV_CMD_CHIP_MI308X;
	case CHIP_LAST:
		return AMDGV_CMD_CHIP_LAST;
	default:
		return AMDGV_CMD_CHIP_UNKNOWN;
	}
}

static void __amdgv_get_device_info(struct gim_dev_data *dev_data, struct amdgv_cmd_dev_info *amdgv_dev)
{
	union amdgv_dev_info dev_info;
	uint32_t asic_type;

	amdgv_dev->bdf = dev_data->init_data.info.bdf;
	amdgv_dev->dev_handle =
		hash_64(dev_data->init_data.info.dev_id ^ dev_data->init_data.info.bdf, 64);

	if (!amdgv_gpumon_get_asic_type(dev_data->adev, &asic_type))
		amdgv_dev->asic_type = amd_asic_type_to_amdgv_cmd_asic_type(asic_type);
	else
		amdgv_dev->asic_type = AMDGV_CMD_CHIP_UNKNOWN;

	if (!amdgv_get_dev_info(dev_data->adev, AMDGV_GET_ENABLED_VF_NUM, &dev_info))
		amdgv_dev->vf_num = dev_info.vf.num_enabled_vf;
	else
		amdgv_dev->vf_num =  0;

	if (amdgv_gpumon_get_ecc_support_flag(dev_data->adev, &amdgv_dev->ecc_enabled,
			&amdgv_dev->ecc_supported)) {
				amdgv_dev->ecc_enabled = 0;
				amdgv_dev->ecc_supported = 0;
	}
}

static int amdgv_get_devices_info(struct amdgv_cmd_devices_info *output_data)
{
	struct gim_dev_data *dev_data;
	uint8_t i = 0;

	mutex_lock(&gim_device_list_lock);
	list_for_each_entry(dev_data, &gim_device_list, list)
		__amdgv_get_device_info(dev_data, &output_data->devs[i++]);

	mutex_unlock(&gim_device_list_lock);
	output_data->dev_num = i;
	return AMDGV_CMD__SUCCESS;
}

static int amdgv_get_ecc_info(struct amdgv_cmd_dev_block_info *input_data, struct amdgv_cmd_ecc_count *output_data)
{
	struct amdgv_smi_ras_query_if query_info = {0};
	amdgv_dev_t adev = NULL;

	query_info.head.block = input_data->block_id;
	query_info.head.sub_block_index = input_data->subblock_id;

	adev = gim_get_dev(input_data->dev.dev_handle);
	if (amdgv_gpumon_get_ecc_info(adev, &query_info))
		return AMDGV_CMD__ERROR_GENERIC;

	output_data->corr_error_cnt = query_info.ce_count;
	output_data->uncorr_error_cnt = query_info.ue_count;
	output_data->deferred_error_cnt = query_info.de_count;

	return AMDGV_CMD__SUCCESS;
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

static int amdgv_ras_ecc_inject(struct amdgv_cmd_ras_inject_error *input_data)
{
	struct amdgv_smi_ras_error_inject_info inject_info = {0};
	amdgv_dev_t adev = NULL;

	inject_info.block_id = amdgv_block_to_ta_block(input_data->device_info.block_id);
	inject_info.sub_block_index = input_data->device_info.subblock_id;
	inject_info.address = input_data->address;
	inject_info.inject_error_type = amdgv_ras_error_type_to_ta_error_type(input_data->error_type);
	inject_info.value = input_data->value;
	inject_info.mask = input_data->mask;

	adev = gim_get_dev(input_data->device_info.dev.dev_handle);
	if (amdgv_gpumon_ras_error_inject(adev, &inject_info))
		return AMDGV_CMD__ERROR_GENERIC;

	return AMDGV_CMD__SUCCESS;
}

static int amdgv_load_ras_ta(struct amdgv_cmd_ras_ta_load_req *input_data,
	struct amdgv_cmd_ras_ta_load_rsp *out_data)
{
	struct amdgv_smi_cmd_ras_ta_load ras_ta_load = {0};
	amdgv_dev_t adev = NULL;
	uint8_t *buf_ptr;

	if (!input_data->version || !input_data->data_len || !input_data->data_addr) {
		gim_warn("Invaild ras ta parameter: version:0x%x, data_len:0x%x, data_addr:0x%llx\n",
			input_data->version, input_data->data_len, input_data->data_addr);
		return AMDGV_CMD__ERROR_GENERIC;
	}

	buf_ptr = gim_oss_interfaces.alloc_memory(input_data->data_len);
	if (!buf_ptr) {
		gim_warn("Failed to alloc memory!\n");
		return AMDGV_CMD__ERROR_GENERIC;
	}

	if (copy_from_user(buf_ptr, (uint8_t *)input_data->data_addr, input_data->data_len)) {
		gim_warn("Failed to copy data from user!\n");
		gim_oss_interfaces.free_memory(buf_ptr);
		return AMDGV_CMD__ERROR_GENERIC;
	}

	adev = gim_get_dev(input_data->dev.dev_handle);
	ras_ta_load.version = input_data->version;
	ras_ta_load.in_data_len = input_data->data_len;
	ras_ta_load.in_data_addr = (uint64_t)buf_ptr;

	if (amdgv_gpumon_ras_ta_load(adev, &ras_ta_load)) {
		gim_warn("Failed to load RAS TA!\n");
		gim_oss_interfaces.free_memory(buf_ptr);
		return AMDGV_CMD__ERROR_GENERIC;
	}

	out_data->ras_session_id = ras_ta_load.out_ras_session_id;

	gim_oss_interfaces.free_memory(buf_ptr);

	return AMDGV_CMD__SUCCESS;
}

static int amdgv_unload_ras_ta(struct amdgv_cmd_ras_ta_unload *input_data)
{
	struct amdgv_smi_cmd_ras_ta_unload ras_ta_unload = {0};
	amdgv_dev_t adev = NULL;

	adev = gim_get_dev(input_data->dev.dev_handle);
	ras_ta_unload.ras_session_id = input_data->ras_session_id;

	if (amdgv_gpumon_ras_ta_unload(adev, &ras_ta_unload))
		return AMDGV_CMD__ERROR_GENERIC;

	return AMDGV_CMD__SUCCESS;
}


static int amdgv_enable_error_injection(struct amdgv_cmd_ras_enable_ecc *input_data)
{
	amdgv_dev_t adev = gim_get_dev(input_data->device.dev_handle);

	if (amdgv_gpumon_turn_on_ecc_injection(adev, input_data->passphrase))
		return AMDGV_CMD__ERROR_GENERIC;

	return AMDGV_CMD__SUCCESS;
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

static uint32_t amdgv_get_bad_pages(struct amdgv_cmd_req_bad_pages_group *input_data,
	 struct amdgv_cmd_bad_pages_info *output_data)
{
	struct amdgv_smi_ras_eeprom_table_record record;
	uint32_t i = 0, bp_cnt = 0, group_cnt = 0;
	amdgv_dev_t *adev = gim_get_dev(input_data->device.dev_handle);

	if (!adev)
		return AMDGV_CMD__ERROR_GENERIC;

	output_data->bp_in_group = 0;
	output_data->group_index = 0;

	amdgv_gpumon_get_bad_page_record_count(adev, &bp_cnt);
	if (bp_cnt) {
		output_data->group_index = input_data->group_index;
		group_cnt = bp_cnt / AMDGV_CMD_MAX_BAD_PAGES_PER_GROUP
			+ ((bp_cnt % AMDGV_CMD_MAX_BAD_PAGES_PER_GROUP) ? 1 : 0);

		if (input_data->group_index >= group_cnt)
			return AMDGV_CMD__ERROR_INVALID_INPUT;

		i = input_data->group_index * AMDGV_CMD_MAX_BAD_PAGES_PER_GROUP;
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

static int amdgv_clear_bad_page_info(struct amdgv_cmd_dev_handle *input_data)
{
	amdgv_dev_t *adev = gim_get_dev(input_data->dev_handle);

	if (amdgv_gpumon_ras_eeprom_clear(adev))
		return AMDGV_CMD__ERROR_GENERIC;

	return AMDGV_CMD__SUCCESS;
}

static int amdgv_force_reset(struct amdgv_cmd_dev_handle *input_data)
{
	amdgv_dev_t *adev = gim_get_dev(input_data->dev_handle);
	if (amdgv_force_reset_gpu(adev))
		return AMDGV_CMD__ERROR_GENERIC;

	return AMDGV_CMD__SUCCESS;
}

static int amdgv_get_pf_region_info(struct amdgv_cmd_dev_handle *input_data, struct amdgv_cmd_fb_regions *output_data)
{
	struct amdgv_fb_regions fb_regions;
	amdgv_dev_t *adev = gim_get_dev(input_data->dev_handle);

	if (!adev)
		return AMDGV_CMD__ERROR_DRV_INIT_FAIL;

	if (amdgv_get_fb_regions_info(adev, 0, &fb_regions))
		return AMDGV_CMD__ERROR_GENERIC;

	output_data->region_areas[0].region.pf = AMDGV_FB_REGION_PF_DATA_EXCHANGE;
	output_data->region_areas[0].start = fb_regions.pf_dataexchange.offset;
	output_data->region_areas[0].size = fb_regions.pf_dataexchange.size;

	output_data->region_areas[1].region.pf = AMDGV_FB_REGION_CSA;
	output_data->region_areas[1].start = fb_regions.csa.offset;
	output_data->region_areas[1].size = fb_regions.csa.size;

	output_data->region_areas[2].region.pf = AMDGV_FB_REGION_TMR;
	output_data->region_areas[2].start = fb_regions.tmr.offset;
	output_data->region_areas[2].size = fb_regions.tmr.size;

	output_data->region_areas[3].region.pf = AMDGV_FB_REGION_PF_IP_DISCOVERY;
	output_data->region_areas[3].start = fb_regions.pf_ipd.offset;
	output_data->region_areas[3].size = fb_regions.pf_ipd.size;

	output_data->reg_cnt = 4;
	return AMDGV_CMD__SUCCESS;
}

static int amdgv_get_vf_region_info(struct amdgv_cmd_vf_info *input_data, struct amdgv_cmd_fb_regions *output_data)
{
	struct amdgv_fb_regions fb_regions;
	union amdgv_dev_info dev_info;
	amdgv_dev_t *adev = gim_get_dev(input_data->device.dev_handle);
	struct gim_dev_data *dev_data = gim_get_dev_data(adev);

	if (amdgv_get_dev_info(adev, AMDGV_GET_ENABLED_VF_NUM, &dev_info))
		return AMDGV_CMD__ERROR_GENERIC;

	if (input_data->vf_index >= dev_info.vf.num_enabled_vf)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	if (amdgv_get_vf_info(adev, input_data->vf_index, AMDGV_GET_VF_FB, &dev_data->vf_info))
		return AMDGV_CMD__ERROR_GENERIC;

	output_data->region_areas[0].region.vf = AMDGV_FB_REGION_VF;
	output_data->region_areas[0].start = MBYTES_TO_BYTES(dev_data->vf_info.fb.fb_offset);
	output_data->region_areas[0].size = MBYTES_TO_BYTES(dev_data->vf_info.fb.fb_size);

	if (amdgv_get_fb_regions_info(adev, input_data->vf_index, &fb_regions))
		return AMDGV_CMD__ERROR_GENERIC;

	output_data->region_areas[1].region.vf = AMDGV_FB_REGION_VF_DATA_EXCHANGE;
	output_data->region_areas[1].start = fb_regions.vf_dataexchange.offset;
	output_data->region_areas[1].size = fb_regions.vf_dataexchange.size;

	output_data->region_areas[2].region.vf = AMDGV_FB_REGION_VF_IP_DISCOVERY;
	output_data->region_areas[2].start = fb_regions.vf_ipd.offset;
	output_data->region_areas[2].size = fb_regions.vf_ipd.size;

	output_data->reg_cnt = 3;
	return AMDGV_CMD__SUCCESS;
}

static int amdgv_get_debug_data(struct amdgv_cmd_debug_data_dir_path *input_data)
{
	struct file *file;
	uint8_t save_path[AMDGV_CMD_PATH_LEN];
	char dir_save_path[AMDGV_CMD_PATH_LEN] = AMDGV_CMD_DIAG_DATA_DEF_PATH;
	uint32_t debug_data_size = AMDGV_CMD_MAX_DIAG_DATA_BUFFER_SIZE;
	uint32_t bdf = 0, ret = 0;
	amdgv_dev_t adev = NULL;
	struct gim_dev_data *dev_data = NULL;
	void *debug_data = gim_oss_interfaces.alloc_memory(debug_data_size);

	if (!debug_data) {
		return -ENOMEM;
	}

	if (strlen(input_data->abs_dir_path) > 0)
		strcpy(dir_save_path, input_data->abs_dir_path);

	mutex_lock(&gim_device_list_lock);
	dev_data = list_first_entry(&gim_device_list, typeof(*dev_data), list);
	adev = dev_data->adev;
	bdf = dev_data->init_data.info.bdf;
	mutex_unlock(&gim_device_list_lock);

	while (&dev_data->list != &gim_device_list) {
		ret = amdgv_get_diag_data(adev, bdf,
				debug_data, &debug_data_size);
		if (ret)
			goto out;

		if (debug_data_size) {
			snprintf(save_path, strlen(dir_save_path) + 30, "%s/%s_%02x_%02x_%02x",
					dir_save_path, "gim_debug_data_gpu", bdf >> 8 & 0xff, bdf >> 3 & 0x1f, bdf & 0x7);

			file = filp_open(save_path, O_CREAT | O_WRONLY, 0444);
			if (IS_ERR_OR_NULL(file)) {
				ret = PTR_ERR(file);
				goto out;
			}
			ret = kernel_write(file, debug_data, debug_data_size, 0);
			filp_close(file, NULL);
			if (ret != debug_data_size) {
				ret = (int)ret == 0 ? -EINVAL : (int)ret;
				goto out;
			}
		}

		mutex_lock(&gim_device_list_lock);
		dev_data = list_next_entry(dev_data, list);
		adev = dev_data->adev;
		bdf = dev_data->init_data.info.bdf;
		mutex_unlock(&gim_device_list_lock);
		debug_data_size = AMDGV_CMD_MAX_DIAG_DATA_BUFFER_SIZE;
	}
	ret = AMDGV_CMD__SUCCESS;

out:
	gim_oss_interfaces.free_memory(debug_data);
	return ret;
}

static int amdgv_get_vf_bdf(struct amdgv_cmd_vf_info *input_data, union amdgv_cmd_device_bdf *output_data)
{
	union amdgv_dev_info dev_info;
	amdgv_dev_t *adev = gim_get_dev(input_data->device.dev_handle);
	struct gim_dev_data *dev_data = gim_get_dev_data(adev);

	if (amdgv_get_dev_info(adev, AMDGV_GET_ENABLED_VF_NUM, &dev_info))
		return AMDGV_CMD__ERROR_GENERIC;

	if (input_data->vf_index >= dev_info.vf.num_enabled_vf)
		return AMDGV_CMD__ERROR_INVALID_INPUT;

	if (amdgv_get_vf_info(adev, input_data->vf_index, AMDGV_GET_VF_BDF, &dev_data->vf_info))
		return AMDGV_CMD__ERROR_GENERIC;

	output_data->u32_all = dev_data->vf_info.id.bdf;

	return AMDGV_CMD__SUCCESS;
}

static int amdgv_ioctl_set_bp_mode(struct amdgv_cmd_set_bp_mode *input_data)
{
	amdgv_dev_t *adev = gim_get_dev(input_data->dev.dev_handle);

	if (amdgv_set_bp_mode(adev, input_data->bp_mode))
		return AMDGV_CMD__ERROR_GENERIC;

	return AMDGV_CMD__SUCCESS;
}

static int amdgv_ioctl_get_bp_mode(struct amdgv_cmd_dev_handle *input_data, enum amdgv_cmd_bp_mode *output_data)
{
	amdgv_dev_t *adev = gim_get_dev(input_data->dev_handle);
	int bp_mode = amdgv_get_bp_mode(adev);

	memcpy(output_data, &bp_mode, sizeof(enum amdgv_cmd_bp_mode));

	return AMDGV_CMD__SUCCESS;
}

static int amdgv_ioctl_get_current_bp(struct amdgv_cmd_dev_handle *input_data, struct amdgv_cmd_bp_info *output_data)
{
	amdgv_dev_t *adev = gim_get_dev(input_data->dev_handle);

	if (amdgv_get_current_bp(adev, (struct amdgv_bp_info *)output_data))
		return AMDGV_CMD__ERROR_GENERIC;

	return AMDGV_CMD__SUCCESS;
}

static int amdgv_ioctl_bp_go(struct amdgv_cmd_dev_handle *input_data)
{
	amdgv_dev_t *adev = gim_get_dev(input_data->dev_handle);

	if (amdgv_bp_go(adev))
		return AMDGV_CMD__ERROR_GENERIC;

	return AMDGV_CMD__SUCCESS;
}

static int amdgv_ioctl_send_ws_cmd(struct amdgv_cmd_user_ws_cmd *input_data)
{
	amdgv_dev_t *adev = gim_get_dev(input_data->dev.dev_handle);

	if (amdgv_send_ws_cmd(adev, input_data->ws_cmd, input_data->hw_sched_id, input_data->idx_vf))
		return AMDGV_CMD__ERROR_GENERIC;

	return AMDGV_CMD__SUCCESS;
}

static void __amdgv_get_device_ex_info(struct gim_dev_data *dev_data, struct amdgv_cmd_dev_ex_info *amdgv_dev)
{
	union amdgv_dev_info dev_info;
	uint32_t asic_type;

	amdgv_dev->bdf = dev_data->init_data.info.bdf;
	amdgv_dev->dev_handle =
		hash_64(dev_data->init_data.info.dev_id ^ dev_data->init_data.info.bdf, 64);

	if (!amdgv_gpumon_get_asic_type(dev_data->adev, &asic_type))
		amdgv_dev->asic_type = amd_asic_type_to_amdgv_cmd_asic_type(asic_type);
	else
		amdgv_dev->asic_type = AMDGV_CMD_CHIP_UNKNOWN;

	if (!amdgv_get_dev_info(dev_data->adev, AMDGV_GET_ENABLED_VF_NUM, &dev_info))
		amdgv_dev->vf_num = dev_info.vf.num_enabled_vf;
	else
		amdgv_dev->vf_num =  0;

	if (!amdgv_get_dev_info(dev_data->adev, AMDGV_GET_OAM_IDX, &dev_info))
		amdgv_dev->oam_id = dev_info.oam.oam_idx;
	else
		amdgv_dev->oam_id = -1;

	if (amdgv_gpumon_get_ecc_support_flag(dev_data->adev, &amdgv_dev->ecc_enabled,
			&amdgv_dev->ecc_supported)) {
				amdgv_dev->ecc_enabled = 0;
				amdgv_dev->ecc_supported = 0;
	}
}

static int amdgv_get_devices_ex_info(struct amdgv_cmd_devices_ex_info *output_data)
{
	struct gim_dev_data *dev_data;
	uint8_t i = 0;

	mutex_lock(&gim_device_list_lock);
	list_for_each_entry(dev_data, &gim_device_list, list)
		__amdgv_get_device_ex_info(dev_data, &output_data->devs[i++]);

	mutex_unlock(&gim_device_list_lock);
	output_data->dev_num = i;
	return AMDGV_CMD__SUCCESS;
}

static const struct file_operations amdgv_cmd_file_ops = {
	.owner                  = THIS_MODULE,
	.unlocked_ioctl         = amdgv_ioctl_handler,
	.open                   = simple_open
};

static long amdgv_ioctl_handler(struct file *file, unsigned int cmd, unsigned long arg)
{

	if (cmd == AMDGV_CMD) {
		if (copy_from_user(amdgv_cmd, (void *) arg, sizeof(struct amdgv_cmd)))
			return -EFAULT;

		if (amdgv_cmd->version != AMDGV_CMD_VERSION &&
			amdgv_cmd->version != AMDGV_CMD_VERSION_V2)
			amdgv_cmd->cmd_res = AMDGV_CMD__ERROR_VERSION;
		else {
			amdgv_cmd->cmd_res = AMDGV_CMD__ERROR_INVALID_INPUT;

			switch (amdgv_cmd->cmd_id) {
			case AMDGV_CMD_GET_DEVICES_INFO:
				amdgv_cmd->output_size = sizeof(struct amdgv_cmd_devices_info);
				amdgv_cmd->cmd_res = amdgv_get_devices_info((struct amdgv_cmd_devices_info *) amdgv_cmd->output_buff_raw);
				break;
			case AMDGV_CMD_GET_BLOCK_ECC_STATUS:
				/*
				 * For get block ecc status the command version must be set V2
				 * due to support deferred error count, the output buff raw size
				 * is changed.
				 */
				if (amdgv_cmd->version != AMDGV_CMD_VERSION_V2) {
					amdgv_cmd->cmd_res = AMDGV_CMD__ERROR_VERSION;
					break;
				}
				amdgv_cmd->output_size = sizeof(struct amdgv_cmd_ecc_count);
				if (amdgv_cmd->input_size == sizeof(struct amdgv_cmd_dev_block_info))
					amdgv_cmd->cmd_res = amdgv_get_ecc_info((struct amdgv_cmd_dev_block_info *) amdgv_cmd->input_buff_raw,
						(struct amdgv_cmd_ecc_count *) amdgv_cmd->output_buff_raw);
				break;
			case AMDGV_CMD_RAS_INJECT_ERROR:
				amdgv_cmd->output_size = 0;
				if (amdgv_cmd->input_size == sizeof(struct amdgv_cmd_ras_inject_error))
					amdgv_cmd->cmd_res = amdgv_ras_ecc_inject((struct amdgv_cmd_ras_inject_error *) amdgv_cmd->input_buff_raw);
				break;
			case AMDGV_CMD_RAS_ENABLE:
				amdgv_cmd->output_size = 0;
				if (amdgv_cmd->input_size == sizeof(struct amdgv_cmd_ras_enable_ecc))
					amdgv_cmd->cmd_res = amdgv_enable_error_injection((struct amdgv_cmd_ras_enable_ecc *) amdgv_cmd->input_buff_raw);
				break;
			case AMDGV_CMD_GET_BAD_PAGES:
				amdgv_cmd->output_size = sizeof(struct amdgv_cmd_bad_pages_info);
				if (amdgv_cmd->input_size == sizeof(struct amdgv_cmd_req_bad_pages_group))
					amdgv_cmd->cmd_res = amdgv_get_bad_pages((struct amdgv_cmd_req_bad_pages_group *) amdgv_cmd->input_buff_raw,
						(struct amdgv_cmd_bad_pages_info *) amdgv_cmd->output_buff_raw);
				break;
			case AMDGV_CMD_CLEAR_BAD_PAGE_INFO:
				amdgv_cmd->output_size = 0;
				if (amdgv_cmd->input_size == sizeof(struct amdgv_cmd_dev_handle))
					amdgv_cmd->cmd_res = amdgv_clear_bad_page_info((struct amdgv_cmd_dev_handle *) amdgv_cmd->input_buff_raw);
				break;
			case AMDGV_CMD_GPU_RESET:
				amdgv_cmd->output_size = 0;
				if (amdgv_cmd->input_size == sizeof(struct amdgv_cmd_dev_handle))
					amdgv_cmd->cmd_res = amdgv_force_reset((struct amdgv_cmd_dev_handle *) amdgv_cmd->input_buff_raw);
				break;
			case AMDGV_CMD_GET_FB_PF_REGIONS:
				amdgv_cmd->output_size = sizeof(struct amdgv_cmd_fb_regions);
				if (amdgv_cmd->input_size == sizeof(struct amdgv_cmd_dev_handle))
					amdgv_cmd->cmd_res = amdgv_get_pf_region_info((struct amdgv_cmd_dev_handle *) amdgv_cmd->input_buff_raw,
						(struct amdgv_cmd_fb_regions *) amdgv_cmd->output_buff_raw);
				break;
			case AMDGV_CMD_GET_FB_VF_REGIONS:
				amdgv_cmd->output_size = sizeof(struct amdgv_cmd_fb_regions);
				if (amdgv_cmd->input_size == sizeof(struct amdgv_cmd_vf_info))
					amdgv_cmd->cmd_res = amdgv_get_vf_region_info((struct amdgv_cmd_vf_info *) amdgv_cmd->input_buff_raw,
						(struct amdgv_cmd_fb_regions *) amdgv_cmd->output_buff_raw);
				break;
			case AMDGV_CMD_GET_DIAG_DATA:
				amdgv_cmd->output_size = 0;
				if (amdgv_cmd->input_size == sizeof(struct amdgv_cmd_debug_data_dir_path)) {
					amdgv_cmd->cmd_res = amdgv_get_debug_data((struct amdgv_cmd_debug_data_dir_path *) amdgv_cmd->input_buff_raw);
				}
				break;
			case AMDGV_CMD_GET_VF_BDF:
				amdgv_cmd->output_size = sizeof(union amdgv_cmd_device_bdf);
				if (amdgv_cmd->input_size == sizeof(struct amdgv_cmd_vf_info)) {
					amdgv_cmd->cmd_res = amdgv_get_vf_bdf((struct amdgv_cmd_vf_info *) amdgv_cmd->input_buff_raw,
						(union amdgv_cmd_device_bdf *) amdgv_cmd->output_buff_raw);
				}
				break;
			case AMDGV_CMD_RAS_TA_LOAD:
				amdgv_cmd->output_size = sizeof(struct amdgv_cmd_ras_ta_load_rsp);
				if (amdgv_cmd->input_size == sizeof(struct amdgv_cmd_ras_ta_load_req))
					amdgv_cmd->cmd_res = amdgv_load_ras_ta((struct amdgv_cmd_ras_ta_load_req *) amdgv_cmd->input_buff_raw,
						(struct amdgv_cmd_ras_ta_load_rsp *) amdgv_cmd->output_buff_raw);
				break;
			case AMDGV_CMD_RAS_TA_UNLOAD:
				amdgv_cmd->output_size = 0;
				if (amdgv_cmd->input_size == sizeof(struct amdgv_cmd_ras_ta_unload))
					amdgv_cmd->cmd_res = amdgv_unload_ras_ta((struct amdgv_cmd_ras_ta_unload *) amdgv_cmd->input_buff_raw);
				break;
			case AMDGV_CMD_SET_BP_MODE:
				amdgv_cmd->output_size = 0;
				if (amdgv_cmd->input_size == sizeof(struct amdgv_cmd_set_bp_mode))
					amdgv_cmd->cmd_res = amdgv_ioctl_set_bp_mode((struct amdgv_cmd_set_bp_mode *) amdgv_cmd->input_buff_raw);
				break;
			case AMDGV_CMD_GET_BP_MODE:
				amdgv_cmd->output_size = sizeof(enum amdgv_cmd_bp_mode);
				if (amdgv_cmd->input_size == sizeof(struct amdgv_cmd_dev_handle))
					amdgv_cmd->cmd_res = amdgv_ioctl_get_bp_mode((struct amdgv_cmd_dev_handle *) amdgv_cmd->input_buff_raw,
						(enum amdgv_cmd_bp_mode *) amdgv_cmd->output_buff_raw);
				break;
			case AMDGV_CMD_GET_CURRENT_BP:
				amdgv_cmd->output_size = sizeof(struct amdgv_cmd_bp_info);
				if (amdgv_cmd->input_size == sizeof(struct amdgv_cmd_dev_handle))
					amdgv_cmd->cmd_res = amdgv_ioctl_get_current_bp((struct amdgv_cmd_dev_handle *) amdgv_cmd->input_buff_raw,
						(struct amdgv_cmd_bp_info *) amdgv_cmd->output_buff_raw);
				break;
			case AMDGV_CMD_BP_GO:
				amdgv_cmd->output_size = 0;
				if (amdgv_cmd->input_size == sizeof(struct amdgv_cmd_dev_handle))
					amdgv_cmd->cmd_res = amdgv_ioctl_bp_go((struct amdgv_cmd_dev_handle *) amdgv_cmd->input_buff_raw);
				break;
			case AMDGV_CMD_SEND_WS_CMD:
				amdgv_cmd->output_size = 0;
				if (amdgv_cmd->input_size == sizeof(struct amdgv_cmd_user_ws_cmd))
					amdgv_cmd->cmd_res = amdgv_ioctl_send_ws_cmd((struct amdgv_cmd_user_ws_cmd *) amdgv_cmd->input_buff_raw);
				break;
			case AMDGV_CMD_GET_DEVICES_EX_INFO:
				amdgv_cmd->output_size = sizeof(struct amdgv_cmd_devices_ex_info);
				amdgv_cmd->cmd_res = amdgv_get_devices_ex_info((struct amdgv_cmd_devices_ex_info *) amdgv_cmd->output_buff_raw);
				break;
			default:
				amdgv_cmd->cmd_res = AMDGV_CMD__ERROR_UKNOWN_CMD;
			}
		}

		if (copy_to_user((void *)arg, amdgv_cmd, sizeof(struct amdgv_cmd)))
			return -EFAULT;
	}
	memset(amdgv_cmd, 0, sizeof(struct amdgv_cmd));
	return 0;
}

int gim_cmd_handler_init(void)
{
	int ret;

	ret = alloc_chrdev_region(&amdgv_dev, 0, AMDGV_CMD_MINOR_COUNT, AMDGV_CMD_HANDLER_NAME);
	if (ret < 0) {
		gim_warn("Cannot allocate major number for IOCTL device\n");
		return ret;
	}
#if !defined(HAVE_CLASS_CREATE_1_ARG)
	amdgv_class = class_create(THIS_MODULE, AMDGV_CMD_HANDLER_NAME);
#else
	amdgv_class = class_create(AMDGV_CMD_HANDLER_NAME);
#endif
	if (amdgv_class == NULL) {
		gim_warn("Cannot create the struct class for IOCTL\n");
		unregister_chrdev_region(amdgv_dev, 1);
		return -EFAULT;
	}

	if ((device_create(amdgv_class, NULL, amdgv_dev, NULL, AMDGV_CMD_HANDLER_NAME)) == NULL) {
		gim_warn("Cannot create the IOCTL device\n");
		class_destroy(amdgv_class);
		unregister_chrdev_region(amdgv_dev, 1);
		return -EFAULT;
	}

	cdev_init(&amdgv_cdev, &amdgv_cmd_file_ops);
	ret = cdev_add(&amdgv_cdev, amdgv_dev, 1);
	if (ret < 0) {
		gim_warn("Cannot add IOCTL device\n");
		gim_cmd_handler_fini();
		return ret;
	}

	amdgv_cmd = vmalloc(sizeof(struct amdgv_cmd));
	if (!amdgv_cmd) {
		gim_warn("Cannot allocate memory for IOCTL command\n");
		gim_cmd_handler_fini();
		return -ENOMEM;
	}

	return 0;
}

void gim_cmd_handler_fini(void)
{
	vfree(amdgv_cmd);
	cdev_del(&amdgv_cdev);
	device_destroy(amdgv_class, amdgv_dev);
	class_destroy(amdgv_class);
	unregister_chrdev_region(amdgv_dev, AMDGV_CMD_MINOR_COUNT);
}

