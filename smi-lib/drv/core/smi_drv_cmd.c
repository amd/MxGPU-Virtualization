/*
 * Copyright (c) 2019-2021 Advanced Micro Devices, Inc. All rights reserved.
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

#include "smi_drv.h"

#include "smi_drv_core.h"
#include "smi_drv_core_api.h"
#include "smi_drv_cmd.h"
#include <amdgv_gpumon.h>
#include <amdgv_error.h>

#include "smi_drv_oss_wrapper.h"
#include "smi_drv_utils.h"

enum smi_error_type {
	ERROR_OTHER		= 1,
	ERROR_ALLOCATION	= 2,
	ERROR_FREE		= 3,
	ERROR_SET_VF		= 4,
	ERROR_SET_VF_NUM	= 5,
	ERROR_CLEAR_VF_FB	= 6,
	ERROR_PARAM_CHECK	= 7,
};

void smi_put_handle(amdgv_dev_t adev, struct smi_ctx *ctx);

/*
 * convert libgv error into SMI error return value
 * we maybe need more SMI error return value
 */
static int smi_convert_ret_value(enum smi_error_type type,
				int libgv_error_subcode)
{
	/* global conversion */
	switch (libgv_error_subcode) {
	case 0:
		return SMI_STATUS_SUCCESS;
	case AMDGV_ERROR_DRIVER_DEV_INIT_FAIL:
		return SMI_STATUS_IO;
	case AMDGV_ERROR_GPU_NOT_INITIALIZED:
		return SMI_STATUS_NOT_INIT;
	case AMDGV_ERROR_DRIVER_GET_READ_SEMA_FAIL:
		return SMI_STATUS_RETRY;
	case AMDGV_ERROR_GPUMON_NOT_SUPPORTED:
		return SMI_STATUS_NOT_SUPPORTED;
	}

	/* type-specific conversion */
	switch (type) {
	case ERROR_OTHER:
		switch (libgv_error_subcode) {
		case AMDGV_ERROR_GPUMON_INVALID_MODE:
			return SMI_STATUS_INVAL;
		case AMDGV_ERROR_GPUMON_SET_ALREADY:
			return SMI_STATUS_SUCCESS;
		case AMDGV_ERROR_GPUMON_VF_BUSY:
			return SMI_STATUS_BUSY;
		default:
			return SMI_STATUS_API_FAILED;
		}
	case ERROR_PARAM_CHECK:
		switch (libgv_error_subcode) {
		default:
			return SMI_STATUS_INVAL;
		}
	case ERROR_ALLOCATION:
		switch (libgv_error_subcode) {
		case AMDGV_ERROR_GPUMON_NO_AVAILABLE_SLOT:
			return SMI_STATUS_NO_SLOT;
		case AMDGV_ERROR_GPUMON_NO_SUITABLE_SPACE:
		case AMDGV_ERROR_GPUMON_OVERSIZE_ALLOCATION:
		case AMDGV_ERROR_GPUMON_OVERLAPPING_FB:
			return SMI_STATUS_OUT_OF_RESOURCES;
		case AMDGV_ERROR_GPUMON_INVALID_FB_SIZE:
		case AMDGV_ERROR_GPUMON_INVALID_GFX_TIMESLICE:
		case AMDGV_ERROR_GPUMON_INVALID_MM_TIMESLICE:
		case AMDGV_ERROR_GPUMON_INVALID_GFX_PART:
		default:
			return SMI_STATUS_INVAL;
		}
	case ERROR_FREE:
	case ERROR_SET_VF:
	case ERROR_SET_VF_NUM:
	case ERROR_CLEAR_VF_FB:
	default:
		switch (libgv_error_subcode) {
		case AMDGV_ERROR_GPUMON_VF_BUSY:
			return SMI_STATUS_BUSY;
		case AMDGV_ERROR_GPUMON_NOT_SUPPORTED:
			return SMI_STATUS_NOT_SUPPORTED;
		default:
			return SMI_STATUS_INVAL;
		}
	}

	return SMI_STATUS_API_FAILED;
}

static amdgv_dev_t smi_get_handle(struct smi_ctx *ctx,
			smi_device_handle_t *dev_id,
			struct smi_device_data *ret_dev_data, bool *dev_busy)
{
	amdgv_dev_t adev = NULL;
	struct smi_device_data dev_data = {0};
	uint32_t i;

	for (i = 0; i < ctx->num_devices; i++) {
		if (dev_id->handle == ctx->devices[i].handle) {
			adev = ctx->devices[i].adev;
			break;
		}
	}

	if (!adev)
		return NULL;

	if (ret_dev_data) {
		smi_get_device_data(adev, ret_dev_data, dev_busy, ctx);
		adev = ret_dev_data->adev ? adev : NULL;
	} else {
		smi_get_device_data(adev, &dev_data, dev_busy, ctx);
		adev = dev_data.adev ? adev : NULL;
	}

	return adev;
}

void smi_put_handle(amdgv_dev_t adev, struct smi_ctx *ctx)
{
	put_handle(adev, ctx);
}

/*
 * Dummy function for testing
 */
int smi_dummy_cmd(struct smi_ctx *ctx, void *inb, void *outb,
			  uint16_t in_len, uint16_t out_len)
{
	return SMI_STATUS_NOT_SUPPORTED;
}

int smi_get_server_static_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_server_static_info *info = NULL;
	union amdgv_smi_query_info *smi_query = NULL;
	uint32_t i;

	/* Check version */
	if ((in_len != 0) || (out_len !=
			sizeof(struct smi_server_static_info)))
		return SMI_STATUS_INVAL;

	info = (struct smi_server_static_info *) outb;
	info->num_devices = ctx->num_devices;

	smi_query = smi_oss_funcs->alloc_small_memory(sizeof(union amdgv_smi_query_info));

	if (smi_query == NULL) {
		return SMI_STATUS_OUT_OF_RESOURCES;
	}

	for (i = 0; i < ctx->num_devices; i++) {
		info->devices[i].bdf.as_uint = ctx->devices[i].bdf;
		info->devices[i].dev_id.handle = ctx->devices[i].handle;
		info->devices[i].failed = true;
		if ((ctx->devices[i].adev)) {
			smi_query->status_info.status = amdgv_get_dev_status(ctx->devices[i].adev);
			if (smi_query->status_info.status == AMDGV_STATUS_HW_INIT)
				info->devices[i].failed = false;
		}
	}

	switch (smi_get_shim_log_level()) {
	case AMDGV_ERROR_LEVEL:
		info->debug_level = SMI_DBGL_ERROR;
		break;
	case AMDGV_WARN_LEVEL:
		info->debug_level = SMI_DBGL_WARN;
		break;
	case AMDGV_DEBUG_LEVEL:
		info->debug_level = SMI_DBGL_DEBUG;
		break;
	default:
		info->debug_level = SMI_DBGL_ERROR;
	};

	smi_oss_funcs->free_small_memory(smi_query);

	return SMI_STATUS_SUCCESS;
}

int smi_get_gpu_vbios_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	struct smi_vbios_info *info = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	struct amdgv_vbios_info vbios;
	int ret = 0;
	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
		(out_len != sizeof(struct smi_vbios_info)))
		return SMI_STATUS_INVAL;
	info = (struct smi_vbios_info *) outb;
	id = (struct smi_device_info *) inb;
	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;
	/* get vbios info */
	ret = amdgv_gpumon_get_vbios_info(adev, &vbios);
	if (ret)
		goto end;
	smi_oss_funcs->memcpy(
		info->name, vbios.name, STRLEN_VERYLONG);
	smi_oss_funcs->memcpy(
		info->build_date, vbios.date, STRLEN_NORMAL);
	smi_oss_funcs->memcpy(
		info->part_number, vbios.vbios_pn, STRLEN_VERYLONG);
	smi_oss_funcs->memcpy(
		info->version, vbios.vbios_version_string, STRLEN_VERYLONG);
end:
	smi_put_handle(adev, ctx);
	return smi_convert_ret_value(ERROR_OTHER, ret);
}
int smi_get_gpu_board_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	struct smi_board_info *info = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	struct amdgv_product_info *product = NULL;
	struct smi_device_data dev_data = {0};
	int ret = 0;
	uint32_t i;
	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
		(out_len != sizeof(struct smi_board_info)))
		return SMI_STATUS_INVAL;
	info = (struct smi_board_info *) outb;
	id = (struct smi_device_info *) inb;
	adev = smi_get_handle(ctx, &id->dev_id, &dev_data, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;
	product = smi_oss_funcs->alloc_memory(sizeof(struct amdgv_product_info));
	if (product == NULL) {
		smi_put_handle(adev, ctx);
		return SMI_STATUS_OUT_OF_RESOURCES;
	}
	ret = amdgv_gpumon_get_product_info(adev, product);
	if (ret || !(product->valid)) {
		if (dev_data.parent >= 0) {
			for (i = 0; i < ctx->num_devices; i++) {
				if ((ctx->devices[i].parent == dev_data.parent) &&
						(adev != ctx->devices[i].adev)) {
					ret = amdgv_gpumon_get_product_info(ctx->devices[i].adev,
						product);
					if (!ret && product->valid) {
						break;
					}
				}
			}
		}
	}
	if (product->valid) {
		smi_oss_funcs->memcpy(
			info->model_number, product->model_number, STRLEN_VERYLONG);
		smi_oss_funcs->memcpy(
			info->product_serial, product->product_serial, STRLEN_VERYLONG);
		smi_oss_funcs->memcpy(
			info->fru_id, product->fru_id, STRLEN_VERYLONG);
		smi_oss_funcs->memcpy(
			info->product_name, product->product_name, STRLEN_VERYLONG);
		smi_oss_funcs->memcpy(
			info->manufacturer_name, product->manufacturer_name, STRLEN_VERYLONG);
	}
	smi_oss_funcs->free_memory(product);
	smi_put_handle(adev, ctx);
	return smi_convert_ret_value(ERROR_OTHER, ret);
}
int smi_get_gpu_asic_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	struct smi_asic_info *info = NULL;
	amdgv_dev_t *adev = NULL;
	struct smi_device_data dev_data = {0};
	union amdgv_dev_info dev_info;
	bool dev_busy = false;
	struct amdgv_init_data *init_data = NULL;
	int ret = 0;
	uint64_t asic_serial;
	const char *marketing_name = NULL;
	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
		(out_len != sizeof(struct smi_asic_info)))
		return SMI_STATUS_INVAL;
	info = (struct smi_asic_info *) outb;
	id = (struct smi_device_info *) inb;
	adev = smi_get_handle(ctx, &id->dev_id, &dev_data, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;
	init_data = &dev_data.init_data;
	/* Fill the data */
	info->vendor_id = init_data->info.vendor_id;
	info->device_id = init_data->info.dev_id;
	info->rev_id = init_data->info.rev_id;
	/* get marketing name */
	marketing_name = amdgv_get_market_name(init_data->info.dev_id, init_data->info.rev_id);
	smi_oss_funcs->memcpy(info->market_name, marketing_name, smi_oss_funcs->strlen(marketing_name) + 1);
	ret = amdgv_get_dev_info(adev, AMDGV_GET_OAM_IDX, &dev_info);
	if (ret) {
		smi_put_handle(adev, ctx);
		return SMI_STATUS_API_FAILED;
	}
	info->oam_id = dev_info.oam.oam_idx;
	info->subvendor_id = init_data->info.sub_vnd_id;
	info->num_of_compute_units = SMI_NOT_SUPPORTED;
	info->target_graphics_version = SMI_NOT_SUPPORTED;
	info->subsystem_id = init_data->info.sub_sys_id;
	ret = amdgv_gpumon_get_asic_serial(adev, &asic_serial);
	if (ret) {
		smi_put_handle(adev, ctx);
		return SMI_STATUS_API_FAILED;
	}
	smi_vsnprintf(info->asic_serial, SMI_MAX_STRING_LENGTH, "0x%llX", asic_serial);
	/* only master GPU has product info */
	smi_put_handle(adev, ctx);
	return smi_convert_ret_value(ERROR_OTHER, ret);
}
int smi_get_gpu_vram_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	struct smi_vram_info *info = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	struct amdgv_gpumon_vram_info vram_info;
	int ret = 0;
	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
		(out_len != sizeof(struct smi_vram_info)))
		return SMI_STATUS_INVAL;
	info = (struct smi_vram_info *) outb;
	id = (struct smi_device_info *) inb;
	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;
	ret = amdgv_gpumon_get_vram_info(adev, &vram_info);
	if (ret == 0) {
		info->vram_size = vram_info.vram_size_mb;
		info->vram_type = smi_map_vram_type(vram_info.vram_type);
		info->vram_vendor = smi_map_vram_vendor(vram_info.vram_vendor);
	} else if (ret == AMDGV_ERROR_GPUMON_NOT_SUPPORTED) {
		info->vram_size = SMI_NOT_SUPPORTED;
		ret = SMI_STATUS_SUCCESS;
	} else {
		goto end;
	}
end:
	smi_put_handle(adev, ctx);
	return smi_convert_ret_value(ERROR_OTHER, ret);
}
int smi_get_gpu_driver_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	struct smi_driver_info *info = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	int ret = 0;
	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
		(out_len != sizeof(struct smi_driver_info)))
		return SMI_STATUS_INVAL;
	info = (struct smi_driver_info *) outb;
	id = (struct smi_device_info *) inb;
	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;
	ret = smi_get_driver_version(adev, &info->version_len, info->version);
	if (ret) {
		smi_put_handle(adev, ctx);
		return SMI_STATUS_API_FAILED;
	}
	ret = smi_get_driver_date(adev, info->driver_date);
	if (ret) {
		smi_put_handle(adev, ctx);
		return SMI_STATUS_API_FAILED;
	}
	ret = smi_get_driver_id(&info->id);
	if (ret) {
		smi_put_handle(adev, ctx);
		return SMI_STATUS_API_FAILED;
	}
	smi_put_handle(adev, ctx);
	return SMI_STATUS_SUCCESS;
}
int smi_get_gpu_power_cap_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info_ex *id = NULL;
	struct smi_power_cap_info *info = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	int ret = 0;
	/* Check version */
	if ((in_len != sizeof(struct smi_device_info_ex)) ||
		(out_len != sizeof(struct smi_power_cap_info)))
		return SMI_STATUS_INVAL;
	info = (struct smi_power_cap_info *) outb;
	id = (struct smi_device_info_ex *) inb;
	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;
	/* get info from gpumon */
	ret = amdgv_gpumon_get_dpm_cap(adev, (int *)&info->dpm_cap);
	if (ret)
		goto end;
	ret = amdgv_gpumon_get_gpu_power_capacity(adev, (int *)&info->power_cap);
	if (ret)
		goto end;
	ret = amdgv_gpumon_get_max_configurable_power_limit(adev, (int *)&info->max_power_cap);
	if (ret == AMDGV_ERROR_GPUMON_NOT_SUPPORTED) {
		info->max_power_cap = SMI_NOT_SUPPORTED;
		ret = SMI_STATUS_SUCCESS;
	} else if (ret != SMI_STATUS_SUCCESS)
		goto end;
	else
		ret = SMI_STATUS_SUCCESS;
	ret = amdgv_gpumon_get_min_power_limit(adev, (int *)&info->min_power_cap);
	if (ret == AMDGV_ERROR_GPUMON_NOT_SUPPORTED) {
		info->min_power_cap = SMI_NOT_SUPPORTED;
		ret = SMI_STATUS_SUCCESS;
	} else if (ret != SMI_STATUS_SUCCESS)
		goto end;
end:
	smi_put_handle(adev, ctx);
	return smi_convert_ret_value(ERROR_OTHER, ret);
}
int smi_get_gpu_fb_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	struct smi_pf_fb_info *info = NULL;
	union amdgv_dev_info dev_info;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	int ret = 0;
	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
		(out_len != sizeof(struct smi_pf_fb_info)))
		return SMI_STATUS_INVAL;
	info = (struct smi_pf_fb_info *) outb;
	id = (struct smi_device_info *) inb;
	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;
	/*
	* current FB layout
	* |            PF                |   VFs... | other |
	* |          pf_size             |          |
	* |        pf_fb_reserved        |          |
	* |              total_fb_size              |
	*/
	ret = amdgv_get_dev_info(adev, AMDGV_GET_FB_LAYOUT, &dev_info);
	if (ret)
		goto end;
	info->total_fb_size = dev_info.layout.total_vf_usable_fb +
		dev_info.layout.vf_usable_fb_offset;
	info->max_vf_fb_usable = dev_info.layout.total_vf_usable_fb;
	info->min_vf_fb_usable = AMDGV_MIN_PF_SIZE;
	info->pf_fb_reserved = dev_info.layout.vf_usable_fb_offset;
	info->pf_fb_offset = 0x0;
	info->fb_alignment = 16;
end:
	smi_put_handle(adev, ctx);
	return smi_convert_ret_value(ERROR_OTHER, ret);
}
int smi_get_gpu_cache_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	struct smi_gpu_cache_info *info = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	struct amdgv_gpumon_gpu_cache_info gpu_cache_info;
	int ret = 0;
	uint32_t i = 0;
	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
		(out_len != sizeof(struct smi_gpu_cache_info)))
		return SMI_STATUS_INVAL;
	info = (struct smi_gpu_cache_info *) outb;
	id = (struct smi_device_info *) inb;
	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;
	ret = amdgv_gpumon_get_gpu_cache_info(adev, &gpu_cache_info);
	if (ret == 0) {
		info->num_cache_types = gpu_cache_info.num_cache_types;
		for (i = 0; i < gpu_cache_info.num_cache_types; i++) {
			info->cache[i].cache_size = gpu_cache_info.cache[i].cache_size_kb;
			info->cache[i].cache_level = gpu_cache_info.cache[i].cache_level;
			info->cache[i].cache_properties = gpu_cache_info.cache[i].flags;
			info->cache[i].max_num_cu_shared = gpu_cache_info.cache[i].max_num_cu_shared;
			info->cache[i].num_cache_instance = gpu_cache_info.cache[i].num_cache_instance;
		}
	} else if (ret == AMDGV_ERROR_GPUMON_NOT_SUPPORTED) {
		info->num_cache_types = SMI_NOT_SUPPORTED;
		ret = SMI_STATUS_SUCCESS;
	}
	smi_put_handle(adev, ctx);
	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_set_gpu_power_cap(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_set_gpu_power_cap *id = NULL;
	struct amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	uint32_t max_power_cap;
	uint32_t min_power_cap;
	int ret = SMI_STATUS_SUCCESS;

	if ((in_len != sizeof(struct smi_set_gpu_power_cap)) ||
		(out_len != 0)) {
		return SMI_STATUS_INVAL;
	}

	id = (struct smi_set_gpu_power_cap *)inb;
	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);

	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	ret = amdgv_gpumon_get_max_configurable_power_limit(adev, (int *)&max_power_cap);
	if (ret != SMI_STATUS_SUCCESS)
		goto end;

	ret = amdgv_gpumon_get_min_power_limit(adev, (int *)&min_power_cap);
	if (ret != SMI_STATUS_SUCCESS)
		goto end;

	if (id->power_cap > max_power_cap || id->power_cap < min_power_cap) {
		smi_put_handle(adev, ctx);
		return SMI_STATUS_INVAL;
	}

	ret = amdgv_gpumon_set_gpu_power_capacity(adev, (int)id->power_cap);

end:
	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_gpu_driver_model(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	struct amdgv_dev_t *adev = NULL;
	struct smi_gpu_driver_model *model = NULL;
	int ret = SMI_STATUS_SUCCESS;
	bool dev_busy = false;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
	    (out_len != sizeof(struct smi_gpu_driver_model))) {
		return SMI_STATUS_INVAL;
	}
	model = (struct smi_gpu_driver_model *) outb;
	id = (struct smi_device_info *) inb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	/* Fill the data */
	ret = smi_get_driver_model(adev, &model->model);

	smi_put_handle(adev, ctx);
	return ret;
}

int smi_get_gpu_fw_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	struct smi_fw_info *info = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;

	union amdgv_smi_query_info *smi_query;
	uint32_t i;
	int ret = 0;
	uint32_t ucode_counter;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
		(out_len != sizeof(struct smi_fw_info)))
		return SMI_STATUS_INVAL;

	info = (struct smi_fw_info *) outb;
	id = (struct smi_device_info *) inb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	smi_query = smi_oss_funcs->alloc_small_memory(sizeof(union amdgv_smi_query_info));

	if (smi_query == NULL) {
		ret = SMI_STATUS_OUT_OF_RESOURCES;
		goto end;
	}

	ret = amdgv_get_fw_version(adev, smi_query);
	if (ret)
		goto end;

	ucode_counter = 0;
	for (i = 0; i < smi_query->firmware_info.fw_num; i++) {
		uint32_t ucode_id = smi_ucode_amdgv_to_smi(smi_query->firmware_info.fw_info[i].id);
		if (ucode_id != SMI_FW_ID__MAX) {
			info->fw_info_list[ucode_counter].fw_id = (uint8_t)ucode_id;
			info->fw_info_list[ucode_counter].fw_version =
				smi_query->firmware_info.fw_info[i].version;
			ucode_counter++;
		}
	}
	info->num_fw_info = (uint8_t)ucode_counter;

end:
	smi_put_handle(adev, ctx);

	smi_oss_funcs->free_small_memory(smi_query);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_gpu_performance_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	struct smi_gpu_performance_info *info = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	struct amdgv_gpumon_metrics metrics;
	int ret = SMI_STATUS_SUCCESS;
	int mm_ret;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info_ex)) || (out_len !=
			sizeof(struct smi_gpu_performance_info)))
		return SMI_STATUS_INVAL;

	info = (struct smi_gpu_performance_info *) outb;
	id = (struct smi_device_info *) inb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	/* Fill the data */
	ret = amdgv_gpumon_get_metrics(adev, &metrics);
	if (ret)
		goto end;

	smi_oss_funcs->memset(info, 0, sizeof(*info));

	info->usage.gfx_activity = metrics.gfx_usage;
	info->usage.umc_activity = metrics.mem_usage;

	info->usage.mm_activity = (metrics.mm_ip_usage[0] + metrics.mm_ip_usage[1])/2;

	info->clock.avg_clk[SMI_CLK_TYPE_GFX] =
		metrics.clocks[AMDGV_PP_CLK_GFX].avg;
	info->clock.avg_clk[SMI_CLK_TYPE_MEM] =
		metrics.clocks[AMDGV_PP_CLK_MEM].avg;
	info->clock.cur_clk[SMI_CLK_TYPE_GFX] =
		metrics.clocks[AMDGV_PP_CLK_GFX].curr;
	info->clock.cur_clk[SMI_CLK_TYPE_MEM] =
		metrics.clocks[AMDGV_PP_CLK_MEM].curr;
	info->clock.cur_clk[SMI_CLK_TYPE_VCLK0] =
		metrics.clocks[AMDGV_PP_CLK_VCLK_0].curr;
	info->clock.cur_clk[SMI_CLK_TYPE_VCLK1] =
		metrics.clocks[AMDGV_PP_CLK_VCLK_1].curr;
	info->clock.avg_clk[SMI_CLK_TYPE_VCLK0] =
		metrics.clocks[AMDGV_PP_CLK_VCLK_0].avg;
	info->clock.avg_clk[SMI_CLK_TYPE_VCLK1] =
		metrics.clocks[AMDGV_PP_CLK_VCLK_1].avg;
	info->clock.cur_clk[SMI_CLK_TYPE_DCLK0] =
		metrics.clocks[AMDGV_PP_CLK_DCLK_0].curr;
	info->clock.cur_clk[SMI_CLK_TYPE_DCLK1] =
		metrics.clocks[AMDGV_PP_CLK_DCLK_1].curr;
	info->clock.avg_clk[SMI_CLK_TYPE_DCLK0] =
		metrics.clocks[AMDGV_PP_CLK_DCLK_0].avg;
	info->clock.avg_clk[SMI_CLK_TYPE_DCLK1] =
		metrics.clocks[AMDGV_PP_CLK_DCLK_1].avg;
	info->clock.clk_deep_sleep[SMI_CLK_TYPE_GFX] =
		metrics.clocks[AMDGV_PP_CLK_GFX].ds_disabled;
	info->clock.clk_deep_sleep[SMI_CLK_TYPE_MEM] =
		metrics.clocks[AMDGV_PP_CLK_MEM].ds_disabled;
	info->clock.clk_deep_sleep[SMI_CLK_TYPE_VCLK0] =
		metrics.clocks[AMDGV_PP_CLK_VCLK_0].ds_disabled;
	info->clock.clk_deep_sleep[SMI_CLK_TYPE_VCLK1] =
		metrics.clocks[AMDGV_PP_CLK_VCLK_1].ds_disabled;
	info->clock.clk_deep_sleep[SMI_CLK_TYPE_DCLK0] =
		metrics.clocks[AMDGV_PP_CLK_DCLK_0].ds_disabled;
	info->clock.clk_deep_sleep[SMI_CLK_TYPE_DCLK1] =
		metrics.clocks[AMDGV_PP_CLK_DCLK_1].ds_disabled;

	ret = amdgv_gpumon_get_max_sclk(adev, &info->clock.max_clk[SMI_CLK_TYPE_GFX]);
	if (ret)
		goto end;


	amdgv_gpumon_get_min_sclk(adev, &info->clock.min_clk[SMI_CLK_TYPE_GFX]);

	ret = amdgv_gpumon_get_max_mclk(adev, &info->clock.max_clk[SMI_CLK_TYPE_MEM]);
	if (ret)
		goto end;

	amdgv_gpumon_get_min_mclk(adev, &info->clock.min_clk[SMI_CLK_TYPE_MEM]);


	mm_ret = amdgv_gpumon_get_max_vclk0(adev, &info->clock.max_clk[SMI_CLK_TYPE_VCLK0]);
	// ret can be not supported for GPUs that don't support MM1 and MM2 domains
	if (mm_ret && mm_ret != AMDGV_ERROR_GPUMON_NOT_SUPPORTED)
		goto end;
	else if (mm_ret == AMDGV_ERROR_GPUMON_NOT_SUPPORTED){
		info->clock.max_clk[SMI_CLK_TYPE_VCLK0] = SMI_NOT_SUPPORTED;
		ret = SMI_STATUS_SUCCESS;
	}

	mm_ret = amdgv_gpumon_get_min_vclk0(adev, &info->clock.min_clk[SMI_CLK_TYPE_VCLK0]);

	// ret can be not supported for GPUs that don't support MM1 and MM2 domains
	if (mm_ret && mm_ret != AMDGV_ERROR_GPUMON_NOT_SUPPORTED)
		goto end;
	else if (mm_ret == AMDGV_ERROR_GPUMON_NOT_SUPPORTED){
		info->clock.min_clk[SMI_CLK_TYPE_VCLK0] = SMI_NOT_SUPPORTED;
		ret = SMI_STATUS_SUCCESS;
	}

	mm_ret = amdgv_gpumon_get_max_vclk1(adev, &info->clock.max_clk[SMI_CLK_TYPE_VCLK1]);
	// ret can be not supported for GPUs that don't support MM1 and MM2 domains
	if (mm_ret && mm_ret != AMDGV_ERROR_GPUMON_NOT_SUPPORTED)
		goto end;
	else if (mm_ret == AMDGV_ERROR_GPUMON_NOT_SUPPORTED){
		info->clock.max_clk[SMI_CLK_TYPE_VCLK1] = SMI_NOT_SUPPORTED;
		ret = SMI_STATUS_SUCCESS;
	}

	mm_ret = amdgv_gpumon_get_min_vclk1(adev, &info->clock.min_clk[SMI_CLK_TYPE_VCLK1]);
	// ret can be not supported for GPUs that don't support MM1 and MM2 domains
	if (mm_ret && mm_ret != AMDGV_ERROR_GPUMON_NOT_SUPPORTED)
		goto end;
	else if (mm_ret == AMDGV_ERROR_GPUMON_NOT_SUPPORTED){
		info->clock.min_clk[SMI_CLK_TYPE_VCLK1] = SMI_NOT_SUPPORTED;
		ret = SMI_STATUS_SUCCESS;
	}

	mm_ret = amdgv_gpumon_get_max_dclk0(adev, &info->clock.max_clk[SMI_CLK_TYPE_DCLK0]);
	// ret can be not supported for GPUs that don't support MM1 and MM2 domains
	if (mm_ret && mm_ret != AMDGV_ERROR_GPUMON_NOT_SUPPORTED)
		goto end;
	else if (mm_ret == AMDGV_ERROR_GPUMON_NOT_SUPPORTED){
		info->clock.max_clk[SMI_CLK_TYPE_DCLK0] = SMI_NOT_SUPPORTED;
		ret = SMI_STATUS_SUCCESS;
	}

	mm_ret = amdgv_gpumon_get_min_dclk0(adev, &info->clock.min_clk[SMI_CLK_TYPE_DCLK0]);
	// ret can be not supported for GPUs that don't support MM1 and MM2 domains
	if (mm_ret && mm_ret != AMDGV_ERROR_GPUMON_NOT_SUPPORTED)
		goto end;
	else if (mm_ret == AMDGV_ERROR_GPUMON_NOT_SUPPORTED){
		info->clock.min_clk[SMI_CLK_TYPE_DCLK0] = SMI_NOT_SUPPORTED;
		ret = SMI_STATUS_SUCCESS;
	}

	mm_ret = amdgv_gpumon_get_max_dclk1(adev, &info->clock.max_clk[SMI_CLK_TYPE_DCLK1]);
	// ret can be not supported for GPUs that don't support MM1 and MM2 domains
	if (mm_ret && mm_ret != AMDGV_ERROR_GPUMON_NOT_SUPPORTED)
		goto end;
	else if (mm_ret == AMDGV_ERROR_GPUMON_NOT_SUPPORTED){
		info->clock.max_clk[SMI_CLK_TYPE_DCLK1] = SMI_NOT_SUPPORTED;
		ret = SMI_STATUS_SUCCESS;
	}

	mm_ret = amdgv_gpumon_get_min_dclk1(adev, &info->clock.min_clk[SMI_CLK_TYPE_DCLK1]);
	// ret can be not supported for GPUs that don't support MM1 and MM2 domains
	if (mm_ret && mm_ret != AMDGV_ERROR_GPUMON_NOT_SUPPORTED)
		goto end;
	else if (mm_ret == AMDGV_ERROR_GPUMON_NOT_SUPPORTED){
		info->clock.min_clk[SMI_CLK_TYPE_DCLK1] = SMI_NOT_SUPPORTED;
		ret = SMI_STATUS_SUCCESS;
	}

	ret = amdgv_gpumon_is_clk_locked(adev, AMDGV_PP_CLK_GFX, &info->clock.clk_locked[SMI_CLK_TYPE_GFX]);
	if (ret == AMDGV_ERROR_GPUMON_NOT_SUPPORTED) {
		info->clock.clk_locked[SMI_CLK_TYPE_GFX] = SMI_NOT_SUPPORTED;
		ret = SMI_STATUS_SUCCESS;
	} else if (ret != SMI_STATUS_SUCCESS)
		goto end;
	else
		ret = SMI_STATUS_SUCCESS;
	info->power.socket_power = metrics.power;

	/* The unit of voltages are in mV */
	info->power.gfx_voltage = metrics.volt_gfx == SMI_NOT_SUPPORTED ? metrics.volt_gfx : metrics.volt_gfx / 100 ;
	info->power.soc_voltage = metrics.volt_soc == SMI_NOT_SUPPORTED ? metrics.volt_soc : metrics.volt_soc / 100 ;
	info->power.mem_voltage = metrics.volt_mem == SMI_NOT_SUPPORTED ? metrics.volt_mem : metrics.volt_mem / 100 ;

	info->temp_shutdown.temp[SMI_TEMPERATURE_TYPE_EDGE] = metrics.temp_edge_shutdown;
	info->temp_shutdown.temp[SMI_TEMPERATURE_TYPE_HOTSPOT] = metrics.temp_hotspot_shutdown;
	info->temp_shutdown.temp[SMI_TEMPERATURE_TYPE_VRAM] = metrics.temp_mem_shutdown;

	info->temp.temp[SMI_TEMPERATURE_TYPE_EDGE] = metrics.temp_edge;
	info->temp.temp[SMI_TEMPERATURE_TYPE_HOTSPOT] = metrics.temp_hotspot;
	info->temp.temp[SMI_TEMPERATURE_TYPE_VRAM] = metrics.temp_mem;
	info->temp.temp[SMI_TEMPERATURE_TYPE_PLX] = metrics.temp_plx;

	info->temp_limit.temp[SMI_TEMPERATURE_TYPE_EDGE] = metrics.temp_edge_limit;
	info->temp_limit.temp[SMI_TEMPERATURE_TYPE_HOTSPOT] = metrics.temp_hotspot_limit;
	info->temp_limit.temp[SMI_TEMPERATURE_TYPE_VRAM] = metrics.temp_mem_limit;

end:
	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_data(struct smi_ctx *ctx, void *inb,
		     void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_data_query *smi_query = NULL;
	union smi_data *info = NULL;
	uint32_t ecc_supported = 0;
	uint32_t ras_caps = 0;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	int ret = SMI_STATUS_SUCCESS;

	/* Check version */
	if ((in_len != sizeof(struct smi_data_query)) || (out_len !=
			sizeof(union smi_data)))
		return SMI_STATUS_INVAL;

	info = (union smi_data *) outb;
	smi_query = (struct smi_data_query *) inb;

	adev = smi_get_handle(ctx, &smi_query->dev.dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	switch (smi_query->type) {
	case SMI_POWER_MANAGEMENT:
		ret = amdgv_gpumon_is_power_management_enabled(adev, &info->power_management_enabled);
		break;
	case SMI_RAS_CAPS:
		ret = amdgv_gpumon_get_ecc_support_flag(adev, &ecc_supported, &ras_caps);
		info->ras_caps = ras_caps;
		break;
	default:
		smi_put_handle(adev, ctx);
		return SMI_STATUS_API_FAILED;
	}

	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_vf_static_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	struct smi_vf_static_info *info = NULL;
	smi_device_handle_t pf;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;

	uint32_t idx_vf;
	struct amdgv_vf_option vf_opt;
	struct amdgv_guard_info guard_info;
	int tmp;
	int ret = 0;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
			(out_len != sizeof(struct smi_vf_static_info)))
		return SMI_STATUS_INVAL;

	info = (struct smi_vf_static_info *) outb;
	id = (struct smi_device_info *) inb;

	pf.handle = make_parent_handle(id->dev_id.handle);
	tmp = smi_get_vf_index(ctx, (struct smi_vf_handle *)&id->dev_id);
	if (tmp < 0)
		return SMI_STATUS_NOT_FOUND;

	idx_vf = (uint32_t) tmp;
	adev = smi_get_handle(ctx, &pf, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	ret = amdgv_get_vf_option(adev, idx_vf, &vf_opt);
	if (ret)
		goto out;

	/* Fill the data */

	ret = amdgv_get_vf_info(adev, idx_vf, AMDGV_GET_VF_BDF, &ctx->vf_info);
	if (ret)
		goto out;

	info->bdf.as_uint = ctx->vf_info.id.bdf;
	info->config.gfx_timeslice = vf_opt.gfx_time_slice;

	guard_info.type = AMDGV_GUARD_ALL;

	ret = amdgv_get_guard_info(adev, idx_vf, &guard_info);
	if (ret)
		goto out;

	info->config.fb.fb_offset = (uint32_t)vf_opt.fb_offset;
	info->config.fb.fb_size = (uint32_t)vf_opt.fb_size;

out:
	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_vf_partition_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	int ret;
	struct smi_device_info *id = NULL;
	struct smi_vf_partition_info *info = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	uint32_t idx_vf;
	union amdgv_dev_info dev_info;
	struct amdgv_vf_option vf_opt;
	int pcie_speed, pcie_width = 0;
	int num_vf_supported = 0;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) || (out_len !=
			sizeof(struct smi_vf_partition_info)))
		return SMI_STATUS_INVAL;

	info = (struct smi_vf_partition_info *) outb;
	id = (struct smi_device_info *) inb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	ret = amdgv_get_dev_info(adev, AMDGV_GET_ENABLED_VF_NUM, &dev_info);
	if (ret)
		goto out;

	info->num_vf_enabled = (uint8_t)dev_info.vf.num_enabled_vf;

	for (idx_vf = 0; idx_vf < info->num_vf_enabled; idx_vf++) {
		ret = amdgv_get_vf_option(adev, idx_vf, &vf_opt);
		if (ret)
			goto out;
		info->partition[idx_vf].id.handle =
			smi_get_vf_handle(ctx, &id->dev_id, idx_vf);
		info->partition[idx_vf].fb.fb_offset = (uint32_t)vf_opt.fb_offset;
		info->partition[idx_vf].fb.fb_size = (uint32_t)vf_opt.fb_size;
	}

	smi_get_pcie_confs(adev, &pcie_speed, &pcie_width, &num_vf_supported);
	info->num_vf_supported = (uint8_t)num_vf_supported;

out:
	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_PARAM_CHECK, ret);
}


int smi_set_vf_partition_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_vf_partition_config *config = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	int ret;

	/* Check version */
	if ((in_len != sizeof(struct smi_vf_partition_config)) ||
	    (out_len != 0))
		return SMI_STATUS_INVAL;

	config = (struct smi_vf_partition_config *) inb;

	adev = smi_get_handle(ctx, &config->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	ret = amdgv_set_vf_number(adev, config->num_vf_enable);
	if (ret)
		goto out;

	ret = amdgv_set_all_vf(adev);
	if (ret)
		goto out;

	/* update VF map */
	if (smi_vf_map_update(ctx, adev)) {
		smi_put_handle(adev, ctx);
		return SMI_STATUS_API_FAILED;
	}

out:
	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_SET_VF_NUM, ret);
}

int smi_clear_vf_fb(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *dev_info = NULL;
	uint8_t pattern = 0x00;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	union amdgv_dev_conf dev_conf;
	uint32_t idx_vf = 0;
	smi_device_handle_t pf;
	int ret;
	int tmp;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
	    (out_len != 0))
		return SMI_STATUS_INVAL;

	dev_info = (struct smi_device_info *) inb;

	pf.handle = make_parent_handle(dev_info->dev_id.handle);
	adev = smi_get_handle(ctx, &pf, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	tmp = smi_get_vf_index(ctx, (struct smi_vf_handle *)&dev_info->dev_id);
	if (tmp < 0) {
		smi_put_handle(adev, ctx);
		return SMI_STATUS_NOT_FOUND;
	}

	idx_vf = (uint32_t) tmp;

	ret = amdgv_get_dev_conf(adev, AMDGV_CONF_ENABLE_CLEAR_VF_FB, &dev_conf);
	if (ret)
		goto out;

	/* store original flag */
	tmp = dev_conf.flag_switch;
	/* turn on clear vf fb and do clear */
	dev_conf.flag_switch = 1;

	ret = amdgv_gpumon_clear_vf_fb(adev, idx_vf, pattern);
	if (ret)
		goto out;
	/* resume to original adapter config */
	dev_conf.flag_switch = tmp;

	ret = amdgv_set_dev_conf(adev, AMDGV_CONF_ENABLE_CLEAR_VF_FB, &dev_conf);
	if (ret)
		goto out;
out:
	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_CLEAR_VF_FB, ret);
}

int smi_get_vf_dynamic_info(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *input = NULL;
	struct smi_vf_data *info = NULL;
	smi_device_handle_t pf;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	int tmp;
	uint32_t idx_vf;
	int ret = 0;
	struct amdgv_guard_info guard;
	struct amdgv_time_log *time_log = NULL;

	int i;

	uint64_t now = smi_oss_funcs->get_time_stamp();

	bool is_vf_shutdown;
	uint64_t running_start;
	uint64_t running_end;

	uint64_t total_active_time;
	uint64_t total_running_time;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
	    (out_len != sizeof(struct smi_vf_data)))
		return SMI_STATUS_INVAL;

	input = (struct smi_device_info *) inb;
	info = (struct smi_vf_data *) outb;

	tmp = smi_get_vf_index(ctx, (struct smi_vf_handle *)&input->dev_id);
	if (tmp < 0)
		return SMI_STATUS_NOT_FOUND;
	idx_vf = (uint32_t) tmp;

	pf.handle = make_parent_handle(input->dev_id.handle);
	adev = smi_get_handle(ctx, &pf, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	ret = amdgv_get_vf_info(adev, idx_vf, AMDGV_GET_VF_TIME_LOG,
				&ctx->vf_info);
	if (ret)
		goto out;

	time_log = &ctx->vf_info.time_log;

	info->sched.flr_count = time_log->reset_count;

	smi_generate_date_string(
		info->sched.last_boot_start,
		time_log->init_start);
	smi_generate_date_string(
		info->sched.last_boot_end,
		time_log->init_end);
	info->sched.boot_up_time =
		time_log->init_end - time_log->init_start;

	smi_generate_date_string(
		info->sched.last_shutdown_start,
		time_log->finish_start);
	smi_generate_date_string(
		info->sched.last_shutdown_end,
		time_log->finish_end);
	info->sched.shutdown_time =
		time_log->finish_end - time_log->finish_start;

	smi_generate_date_string(
		info->sched.last_reset_start,
		time_log->reset_start);
	smi_generate_date_string(
		info->sched.last_reset_end,
		time_log->reset_end);
	info->sched.reset_time =
		time_log->reset_end - time_log->reset_start;

	/* total active time */
	total_active_time  = time_log->cumulative_active_time;

	total_running_time = time_log->cumulative_running_time;

	if (time_log->last_save_end != 0 &&
		time_log->last_save_end <= time_log->last_load_start &&
		time_log->finish_end <= time_log->init_start)
		total_active_time += now - time_log->last_load_start;

	if ((time_log->finish_end == 0 && time_log->init_start != 0) ||
		(time_log->finish_end != 0 && time_log->finish_end <= time_log->init_start))
		total_running_time += now - time_log->init_start;

	smi_generate_time_string(
		info->sched.total_active_time,
		total_active_time);

	smi_generate_time_string(
		info->sched.total_running_time,
		total_running_time
	);

	/* check whether VF is shutdown already */
	running_start = time_log->init_start;
	running_end = time_log->finish_end;
	is_vf_shutdown = running_end >= running_start;

	if (is_vf_shutdown) {
		smi_generate_time_string(
			info->sched.current_running_time, 0);
		smi_generate_time_string(
			info->sched.current_active_time, 0);
	} else {
		if (time_log->last_save_end == 0) {
			total_active_time += now - time_log->last_load_start;
			smi_generate_time_string(
				info->sched.total_active_time,
				total_active_time);
		}
		smi_generate_time_string(
			info->sched.current_running_time,
			now - running_start);
		smi_generate_time_string(
			info->sched.current_active_time,
			total_active_time - time_log->historical_active_time_data);
	}
	ret = amdgv_get_vf_info(adev, idx_vf, AMDGV_GET_VF_SCHED_STATE,
				&ctx->vf_info);
	if (ret)
		goto out;
	if (!amdgv_is_customized_vf_mode(adev) &&
		ctx->vf_info.sched.state == AMDGV_SCHED_AVAIL)
		info->sched.state = SMI_VF_STATE_DEFAULT_AVAILABLE;
	else
		info->sched.state = smi_map_sched_state(ctx->vf_info.sched.state);

	guard.type = AMDGV_GUARD_ALL;
	ret = amdgv_get_guard_info(adev, idx_vf, &guard);
	if (ret)
		goto out;
	info->guard.enabled =
		guard.parm.general.state == AMDGV_GUARD_ENABLED;

	for (i = 0; i < SMI_GUARD_EVENT__MAX; i++) {
		guard.type = i;
		ret = amdgv_get_guard_info(adev, idx_vf, &guard);
		if (ret)
			goto out;

		info->guard.guard[i].state =
			guard.parm.event.state;
		info->guard.guard[i].amount =
			guard.parm.event.amount;
		info->guard.guard[i].interval =
			guard.parm.event.interval;
		info->guard.guard[i].threshold =
			guard.parm.event.threshold;
		info->guard.guard[i].active =
			guard.parm.event.active;
	}
out:
	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_PARAM_CHECK, ret);
}

int smi_create_event_set(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_event_set_config *config = NULL;
	smi_event_handle_t *handle = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	int ret;

	/* Check version */
	if ((in_len != sizeof(struct smi_event_set_config)) ||
	   (out_len != sizeof(smi_event_handle_t)))
		return SMI_STATUS_INVAL;

	config = (struct smi_event_set_config *) inb;
	handle = (smi_event_handle_t *) outb;

	adev = smi_get_handle(ctx, &config->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	ret = smi_event_create(ctx, adev, config);
	smi_put_handle(adev, ctx);
	if (ret < 0)
		return SMI_STATUS_API_FAILED;

	handle->fd = config->event_set.fd;

	return SMI_STATUS_SUCCESS;
}

int smi_read_event_set(struct smi_ctx *ctx, void *inb,
		   void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	struct smi_event_entry *event = NULL;
	int res = 0;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
	   (out_len != sizeof(struct smi_event_entry)))
		return SMI_STATUS_INVAL;

	id = (struct smi_device_info *) inb;
	event = (struct smi_event_entry *) outb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	res = smi_event_read(ctx, adev, id->dev_id.handle, event);
	smi_put_handle(adev, ctx);
	if (res < 0)
		return SMI_STATUS_API_FAILED;

	return SMI_STATUS_SUCCESS;
}

int smi_destroy_event_set(struct smi_ctx *ctx, void *inb,
		   void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	int res = 0;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
	   (out_len != 0))
		return SMI_STATUS_INVAL;

	id = (struct smi_device_info *) inb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	res = smi_event_destroy(ctx, adev, id->dev_id.handle);
	smi_put_handle(adev, ctx);
	if (res < 0)
		return SMI_STATUS_API_FAILED;

	return SMI_STATUS_SUCCESS;
}

int smi_get_ecc_info(struct smi_ctx *ctx, void *inb,
		     void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_ecc_info *info = NULL;
	struct smi_device_info *id = NULL;
	struct amdgv_smi_ras_query_if gv_qif = {0};
	uint64_t total_correctable = 0;
	uint64_t total_uncorrectable = 0;
	uint64_t total_deferred = 0;
	uint32_t num_enabled_blocks = 0;
	int ret;
	int i;

	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
	    (out_len != sizeof(struct smi_ecc_info)))
		return SMI_STATUS_INVAL;

	id = (struct smi_device_info *) inb;
	info = (struct smi_ecc_info *) outb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	for (i = AMDGV_SMI_RAS_BLOCK__UMC; i < AMDGV_SMI_NUM_BLOCK_MAX; i++) {
		gv_qif.head.block = i;
		ret = amdgv_gpumon_get_ecc_info(adev, &gv_qif);
		if (!ret) {
			total_correctable += gv_qif.ce_count;
			total_uncorrectable += gv_qif.ue_count;
			total_deferred += gv_qif.de_count;
			num_enabled_blocks++;
		}
	}

	if (num_enabled_blocks == 0) {
		smi_put_handle(adev, ctx);
		return SMI_STATUS_NOT_SUPPORTED;
	}

	info->err_count.correctable_count = total_correctable;
	info->err_count.uncorrectable_count = total_uncorrectable;
	info->err_count.deferred_count = total_deferred;

	smi_put_handle(adev, ctx);

	return SMI_STATUS_SUCCESS;
}

int smi_get_ecc_block_info(struct smi_ctx *ctx, void *inb,
			   void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_ecc_info *info = NULL;
	struct smi_ras_query_if *qif = NULL;
	struct amdgv_smi_ras_query_if gv_qif = {0};
	int ret;

	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;

	/* Check version */
	if ((in_len != sizeof(struct smi_ras_query_if)) ||
	    (out_len != sizeof(struct smi_ecc_info)))
		return SMI_STATUS_INVAL;

	qif = (struct smi_ras_query_if *) inb;
	info = (struct smi_ecc_info *) outb;

	smi_oss_funcs->memcpy(&gv_qif.head, &qif->head,
			      sizeof(struct amdgv_smi_ras_common_if));

	gv_qif.head.block = smi_map_gpu_block(qif->head.block);
	if (gv_qif.head.block == AMDGV_SMI_NUM_BLOCK_MAX) {
		return SMI_STATUS_API_FAILED;
	}

	adev = smi_get_handle(ctx, &qif->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	ret = amdgv_gpumon_get_ecc_info(adev, &gv_qif);
	if (ret) {
		goto end;
	}

	info->err_count.correctable_count = gv_qif.ce_count;
	info->err_count.uncorrectable_count = gv_qif.ue_count;
	info->err_count.deferred_count = gv_qif.de_count;

end:
	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

static int smi_handle_eeprom_table_record(struct amdgv_smi_ras_eeprom_table_record *records,
		   struct amdgv_smi_ras_eeprom_table_record record)
{
	records->retired_page = record.retired_page;
	records->ts = smi_eeprom_to_utc_format(record.ts);
	records->err_type = record.err_type;
	records->mem_channel = record.mem_channel;
	records->mcumc_id = record.mcumc_id;

	return SMI_STATUS_SUCCESS;
}

int smi_query_bad_page_info(struct smi_ctx *ctx, void *inb,
			    void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_bad_page_info *bad_page_info = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	struct amdgv_smi_ras_eeprom_table_record record = {0};
	struct amdgv_smi_ras_eeprom_table_record *bad_page_records = NULL;
	int bp_cnt = 0;
	int i = 0;
	int ret = SMI_STATUS_SUCCESS;

	/* Check version */
	if ((in_len != sizeof(struct smi_bad_page_info)) ||
	    (out_len != 0))
		return SMI_STATUS_INVAL;

	bad_page_info = (struct smi_bad_page_info *) inb;

	adev = smi_get_handle(ctx, &bad_page_info->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	amdgv_gpumon_get_bad_page_record_count(adev, &bp_cnt);
	if (bp_cnt == 0) {
		smi_put_handle(adev, ctx);
		return SMI_STATUS_NO_DATA;
	}

	if (bp_cnt > (int)bad_page_info->size) {
		bp_cnt = (int)bad_page_info->size;
	}

	bad_page_records = smi_oss_funcs->alloc_memory(bp_cnt * sizeof(struct amdgv_smi_ras_eeprom_table_record));
	if (bad_page_records == NULL) {
		smi_put_handle(adev, ctx);
		return SMI_STATUS_OUT_OF_RESOURCES;
	}

	for (i = 0; i < bp_cnt; i++) {
		ret = amdgv_gpumon_get_bad_page_info(adev, i, &record);
		if (ret)
			goto end;
		/* copy eeprom table record */
		smi_handle_eeprom_table_record(bad_page_records + i, record);
	}
	ret = smi_get_eeprom_table(bad_page_info, in_len, bp_cnt, bad_page_records);
end:
	smi_oss_funcs->free_memory(bad_page_records);
	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_handle_id(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_get_handle_info *info = NULL;
	struct smi_get_handle_resp *resp = NULL;
	uint32_t i, j, idx;

	/* Check version */
	if ((in_len != sizeof(struct smi_get_handle_info)) || (out_len !=
			sizeof(struct smi_get_handle_resp)))
		return SMI_STATUS_INVAL;

	info = (struct smi_get_handle_info *) inb;
	resp = (struct smi_get_handle_resp *) outb;

	for (i = 0; i < ctx->num_devices; i++) {
		if (ctx->devices[i].bdf == info->bdf.as_uint) {
			resp->dev_id.handle = ctx->devices[i].handle;
			resp->vf_id.handle = (uint64_t) 0;
			return SMI_STATUS_SUCCESS;
		}

		for (j = 0; j < SMI_MAX_VF_COUNT; j++) {
			idx = i * SMI_MAX_VF_COUNT + j;
			if (ctx->vf_map[idx].bdf == info->bdf.as_uint) {
				resp->vf_id.handle = ctx->vf_map[idx].handle;
				resp->dev_id.handle = (uint64_t) 0;
				return SMI_STATUS_SUCCESS;
			}
		}

	}

	return SMI_STATUS_NOT_FOUND;
}

int smi_get_guest_data(struct smi_ctx *ctx, void *inb,
		       void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	struct smi_guest_info *guest_info = NULL;
	struct amd_sriov_msg_vf2pf_info *vf2pf_info = NULL;
	smi_device_handle_t pf;
	struct smi_guest_data *guest_data = NULL;
	int idx_vf;
	int ret;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
	    (out_len != sizeof(struct smi_guest_info)))
		return SMI_STATUS_INVAL;

	id = (struct smi_device_info *) inb;
	guest_info = (struct smi_guest_info *) outb;

	pf.handle = make_parent_handle(id->dev_id.handle);
	adev = smi_get_handle(ctx, &pf, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	idx_vf = smi_get_vf_index(ctx, (struct smi_vf_handle *)&id->dev_id);
	if (idx_vf < 0) {
		smi_put_handle(adev, ctx);
		return SMI_STATUS_NOT_FOUND;
	}

	vf2pf_info = smi_oss_funcs->alloc_small_memory(
		sizeof(struct amd_sriov_msg_vf2pf_info));
	if (vf2pf_info == NULL) {
		smi_put_handle(adev, ctx);
		return SMI_STATUS_API_FAILED;
	}

	ret = amdgv_get_vf2pf_info(adev, (uint32_t) idx_vf, vf2pf_info);
	if (ret) {
		smi_oss_funcs->free_small_memory(vf2pf_info);
		smi_put_handle(adev, ctx);
		return smi_convert_ret_value(ERROR_OTHER, ret);
	}

	guest_data = &guest_info->guest_data;

	smi_oss_funcs->memcpy(guest_data->driver_version,
			      vf2pf_info->driver_version, sizeof(vf2pf_info->driver_version));
	guest_data->fb_usage = vf2pf_info->fb_usage;

	smi_oss_funcs->free_small_memory(vf2pf_info);
	smi_put_handle(adev, ctx);

	return SMI_STATUS_SUCCESS;
}

int smi_get_dfc_fw(struct smi_ctx *ctx, void *inb,
			      void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	struct smi_dfc_fw *dfc_fw_info = NULL;
	union amdgv_smi_query_info *smi_query = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	int ret;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
	    (out_len != sizeof(struct smi_dfc_fw)))
		return SMI_STATUS_INVAL;

	id = (struct smi_device_info *) inb;
	dfc_fw_info = (struct smi_dfc_fw *) outb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	smi_query = smi_oss_funcs->alloc_small_memory(sizeof(union amdgv_smi_query_info));

	if (smi_query == NULL) {
		return SMI_STATUS_OUT_OF_RESOURCES;
	}

	ret = amdgv_get_smi_info(adev, AMDGV_SMI_DFC_TABLE, smi_query);
	if (ret)
		goto end;

	if (smi_query) {
		smi_oss_funcs->memcpy(&dfc_fw_info->data,
			&smi_query->dfc_fw.data, sizeof(struct smi_dfc_fw_data) * smi_query->dfc_fw.header.dfc_fw_total_entries);
		dfc_fw_info->header.dfc_fw_total_entries =
			smi_query->dfc_fw.header.dfc_fw_total_entries;
		dfc_fw_info->header.dfc_fw_version = smi_query->dfc_fw.header.dfc_fw_version;
	}

end:
	smi_put_handle(adev, ctx);

	smi_oss_funcs->free_small_memory(smi_query);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_pcie_info(struct smi_ctx *ctx, void *inb,
			      void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	struct smi_pcie_info *info = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	struct amdgv_gpumon_metrics metrics;
	enum amdgv_gpumon_card_form_factor type;
	int max_vf_num = 0;
	int ret = SMI_STATUS_SUCCESS;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
	    (out_len != sizeof(struct smi_pcie_info))) {
		return SMI_STATUS_INVAL;
	}

	info = (struct smi_pcie_info *) outb;
	id = (struct smi_device_info *) inb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	/* Fill the data */
	ret = amdgv_gpumon_get_metrics(adev, &metrics);
	if (ret)
		goto end;

	// metric PCIe info
	info->pcie_metric.pcie_width = (uint16_t)metrics.pcie_width;
	info->pcie_metric.pcie_speed = (uint32_t)metrics.pcie_rate;
	info->pcie_metric.pcie_bandwidth = metrics.pcie_bandwidth;
	info->pcie_metric.pcie_replay_count = metrics.pcie_replay_count;
	info->pcie_metric.pcie_l0_to_recovery_count = metrics.pcie_l0_to_recovery_count;
	info->pcie_metric.pcie_replay_roll_over_count = metrics.pcie_replay_roll_over_count;
	info->pcie_metric.pcie_nak_sent_count = metrics.pcie_nak_sent_count;
	info->pcie_metric.pcie_nak_received_count = metrics.pcie_nak_received_count;
	info->pcie_metric.pcie_lc_perf_other_end_recovery_count = SMI_NOT_SUPPORTED;

	// static PCIe info
	smi_get_pcie_confs(adev, &info->pcie_static.max_pcie_speed, (int *) &info->pcie_static.max_pcie_width, &max_vf_num);
	ret = amdgv_gpumon_get_card_form_factor(adev, &type);
	if (ret == 0) {
		info->pcie_static.slot_type = smi_map_card_form_factor(type);
	} else if (ret == AMDGV_ERROR_GPUMON_NOT_SUPPORTED) {
		info->pcie_static.slot_type = SMI_CARD_FORM_FACTOR_UNKNOWN;
		ret = SMI_STATUS_SUCCESS;
	} else {
		goto end;
	}
	ret = amdgv_gpumon_get_gpu_max_pcie_link_generation(adev, &info->pcie_static.max_pcie_interface_version);
	if (ret == AMDGV_ERROR_GPUMON_NOT_SUPPORTED) {
		info->pcie_static.max_pcie_interface_version = SMI_NOT_SUPPORTED;
		ret = SMI_STATUS_SUCCESS;
	} else {
		goto end;
	}

end:
	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_ucode_err_records(struct smi_ctx *ctx, void *inb,
			      void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	struct smi_fw_error_record *records = NULL;
	struct amdgv_psp_mb_err_record err_record[AMDGV_MAX_PSP_MB_ERROR_RECORD];
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	int ret;
	int i;
	uint32_t ucode_counter;
	uint8_t error_record_count;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
	    (out_len != sizeof(struct smi_fw_error_record)))
		return SMI_STATUS_INVAL;

	id = (struct smi_device_info *) inb;
	records = (struct smi_fw_error_record *) outb;
	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;
	ret = amdgv_gpumon_get_fw_err_records(adev, &error_record_count, err_record);
	if (ret)
		goto end;
	ucode_counter = 0;
	for (i = 0; i < error_record_count; i++) {
		uint32_t ucode_id = smi_ucode_amdgv_to_smi(err_record[i].fw_id);
		if (ucode_id != SMI_FW_ID__MAX) {
			records->err_records[ucode_counter].timestamp = err_record[i].timestamp;
			records->err_records[ucode_counter].vf_idx = err_record[i].vf_idx;
			records->err_records[ucode_counter].fw_id = ucode_id;
			records->err_records[ucode_counter].status = (uint16_t)err_record[i].status;
			ucode_counter++;
		}
	}
	records->num_err_records = (uint8_t)ucode_counter;

end:
	smi_put_handle(adev, ctx);
	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_vf_ucode_info(struct smi_ctx *ctx, void *inb,
			  void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	struct smi_fw_info *ucodes = NULL;
	struct amdgv_firmware_info ucode_info[AMDGV_FIRMWARE_ID__MAX];
	uint8_t ucode_info_num = 0;
	smi_device_handle_t pf;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	int ret;
	int i;
	int idx_vf;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
	    (out_len != sizeof(struct smi_fw_info)))
		return SMI_STATUS_INVAL;

	id = (struct smi_device_info *) inb;
	ucodes = (struct smi_fw_info *) outb;

	pf.handle = make_parent_handle(id->dev_id.handle);
	adev = smi_get_handle(ctx, &pf, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	idx_vf = smi_get_vf_index(ctx, (struct smi_vf_handle *)&id->dev_id);
	if (idx_vf < 0) {
		smi_put_handle(adev, ctx);
		return SMI_STATUS_NOT_FOUND;
	}

	smi_oss_funcs->memset(ucode_info, 0,
		sizeof(struct amdgv_firmware_info) * AMDGV_FIRMWARE_ID__MAX);

	ret = amdgv_gpumon_get_vf_fw_info(adev, (uint32_t) idx_vf, &ucode_info_num,
					  ucode_info);
	if (ret) {
		ret = smi_convert_ret_value(ERROR_OTHER, ret);
		goto end;
	}

	if (((int)SMI_FW_ID__MAX < ucode_info_num) || (ucode_info_num >= AMDGV_FIRMWARE_ID__MAX)) {
		ret = SMI_STATUS_API_FAILED;
		goto end;
	}

	ucodes->num_fw_info = 0;
	for (i = 0; i < ucode_info_num; i++) {
		uint32_t smi_ucode_id = smi_ucode_amdgv_to_smi(ucode_info[i].id);
		if (ucode_info[i].id == 0 || smi_ucode_id == SMI_FW_ID__MAX)
			continue;

		ucodes->fw_info_list[ucodes->num_fw_info].fw_id = (uint8_t)smi_ucode_id;
		ucodes->fw_info_list[ucodes->num_fw_info].fw_version = ucode_info[i].version;
		ucodes->num_fw_info++;
	}

end:
	smi_put_handle(adev, ctx);
	return ret;
}

int smi_get_partition_profile_info(struct smi_ctx *ctx, void *inb,
			  void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	struct smi_profile_info *profile_info = NULL;
	struct smi_profile_info *oss_profile_info = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	int ret;
	uint32_t i, j;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
	    (out_len != sizeof(struct smi_profile_info)))
		return SMI_STATUS_INVAL;

	id = (struct smi_device_info *) inb;
	profile_info = (struct smi_profile_info *) outb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	oss_profile_info = smi_oss_funcs->alloc_small_memory(
		sizeof(struct smi_profile_info));

	if (oss_profile_info == NULL) {
		ret = SMI_STATUS_OUT_OF_RESOURCES;
		goto end;
	}

	ret = smi_get_profile_info(adev, oss_profile_info);
	if (ret) {
		if (ret == SMI_STATUS_NOT_SUPPORTED) {
			smi_put_handle(adev, ctx);
			smi_oss_funcs->free_small_memory(oss_profile_info);
			return SMI_STATUS_NOT_SUPPORTED;
		} else {
			ret = smi_convert_ret_value(ERROR_OTHER, ret);
			goto end;
		}
	}

	profile_info->profile_count = (uint8_t)oss_profile_info->profile_count;
	profile_info->current_profile_index = (uint8_t)oss_profile_info->current_profile_index;

	for (i = 0; i < oss_profile_info->profile_count; i++) {
		profile_info->profiles[i].vf_count = oss_profile_info->profiles[i].vf_count;
		for (j = 0; j < SMI_PROFILE_CAPABILITY__MAX; j++) {
			if (j != SMI_PROFILE_CAPABILITY_MEMORY) {
				profile_info->profiles[i].profile_caps[j].total =
						oss_profile_info->profiles[i].profile_caps[j].total;
				profile_info->profiles[i].profile_caps[j].available =
						oss_profile_info->profiles[i].profile_caps[j].available;
				profile_info->profiles[i].profile_caps[j].optimal =
						oss_profile_info->profiles[i].profile_caps[j].optimal;
				profile_info->profiles[i].profile_caps[j].min_value =
						oss_profile_info->profiles[i].profile_caps[j].min_value;
				profile_info->profiles[i].profile_caps[j].max_value =
						oss_profile_info->profiles[i].profile_caps[j].max_value;
			} else {
				profile_info->profiles[i].profile_caps[j].total =
						(oss_profile_info->profiles[i].profile_caps[j].total / 1024) / 1024;
				profile_info->profiles[i].profile_caps[j].available =
						(oss_profile_info->profiles[i].profile_caps[j].available / 1024) / 1024;
				profile_info->profiles[i].profile_caps[j].optimal =
						(oss_profile_info->profiles[i].profile_caps[j].optimal / 1024) / 1024;
				profile_info->profiles[i].profile_caps[j].min_value =
						(oss_profile_info->profiles[i].profile_caps[j].min_value / 1024) / 1024;
				profile_info->profiles[i].profile_caps[j].max_value =
						(oss_profile_info->profiles[i].profile_caps[j].max_value / 1024) / 1024;
			}
		}
	}
end:
	smi_put_handle(adev, ctx);

	smi_oss_funcs->free_small_memory(oss_profile_info);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_link_metrics(struct smi_ctx *ctx, void *inb,
			      void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	struct smi_link_metrics *link = NULL;
	struct amdgv_gpumon_link_metrics *link_metrics = NULL;
	uint32_t i = 0;
	int ret = SMI_STATUS_SUCCESS;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
	    (out_len != sizeof(struct smi_link_metrics))) {
		return SMI_STATUS_INVAL;
	}

	link = (struct smi_link_metrics *) outb;
	id = (struct smi_device_info *) inb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	link_metrics = smi_oss_funcs->alloc_small_memory(sizeof(struct amdgv_gpumon_link_metrics));

	if (link_metrics == NULL) {
		return SMI_STATUS_OUT_OF_RESOURCES;
	}

	ret = amdgv_gpumon_get_link_metrics(adev, link_metrics);
	if (ret)
		goto end;

	link->num_links = link_metrics->num_links;
	for (i = 0; i < link_metrics->num_links; i++) {
		link->links[i].bdf.as_uint = link_metrics->links[i].bdf;
		link->links[i].bit_rate = link_metrics->links[i].speed;
		link->links[i].max_bandwidth = link_metrics->links[i].speed*link_metrics->links[i].width;
		link->links[i].link_type = smi_map_link_type(link_metrics->links[i].link_type);
		link->links[i].read = link_metrics->links[i].read;
		link->links[i].write = link_metrics->links[i].write;
	}

end:
	smi_oss_funcs->free_small_memory(link_metrics);
	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_link_topology(struct smi_ctx *ctx, void *inb,
			      void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_pair_info *id = NULL;
	amdgv_dev_t *src_adev = NULL;
	amdgv_dev_t *dst_adev = NULL;
	bool dev_busy = false;
	struct smi_link_topology *link = NULL;
	struct amdgv_gpumon_link_topology_info link_topology;
	int ret = SMI_STATUS_SUCCESS;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_pair_info)) ||
	    (out_len != sizeof(struct smi_link_topology))) {
		return SMI_STATUS_INVAL;
	}

	link = (struct smi_link_topology *) outb;
	id = (struct smi_device_pair_info *) inb;

	src_adev = smi_get_handle(ctx, &id->src.dev_id, NULL, &dev_busy);
	if (!src_adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	dst_adev = smi_get_handle(ctx, &id->dst.dev_id, NULL, &dev_busy);
	if (!dst_adev) {
		smi_put_handle(src_adev, ctx);
		return SMI_STATUS_NOT_FOUND;
	}

	if (dev_busy) {
		smi_put_handle(src_adev, ctx);
		return SMI_STATUS_BUSY;
	}

	ret = amdgv_gpumon_get_link_topology(src_adev, dst_adev, &link_topology);
	if (ret)
		goto end;

	link->weight = link_topology.weight;
	link->link_status = smi_map_link_status(link_topology.link_status);
	link->link_type = smi_map_link_type(link_topology.link_type);
	link->num_hops = (uint8_t)link_topology.num_hops;
	link->fb_sharing = (uint8_t)link_topology.is_fb_sharing_enabled;

end:
	smi_put_handle(src_adev, ctx);
	smi_put_handle(dst_adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_xgmi_fb_sharing_caps(struct smi_ctx *ctx, void *inb,
			      void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	union smi_xgmi_fb_sharing_caps *xgmi = NULL;
	union amdgv_gpumon_xgmi_fb_sharing_caps xgmi_caps;
	int ret = SMI_STATUS_SUCCESS;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
	    (out_len != sizeof(union smi_xgmi_fb_sharing_caps))) {
		return SMI_STATUS_INVAL;
	}

	xgmi = (union smi_xgmi_fb_sharing_caps *) outb;
	id = (struct smi_device_info *) inb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	ret = amdgv_gpumon_get_xgmi_fb_sharing_caps(adev, &xgmi_caps);
	if (ret)
		goto end;

	xgmi->xgmi_fb_sharing_cap_mask = xgmi_caps.xgmi_fb_sharing_cap_mask;
end:
	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_xgmi_fb_sharing_mode_info(struct smi_ctx *ctx, void *inb,
			      void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_xgmi_fb_sharing *id = NULL;
	amdgv_dev_t *src_adev = NULL;
	amdgv_dev_t *dst_adev = NULL;
	bool dev_busy = false;
	struct smi_xgmi_fb_sharing_flag *xgmi = NULL;
	enum amdgv_gpumon_xgmi_fb_sharing_mode fb_sharing_mode;
	int ret = SMI_STATUS_SUCCESS;

	/* Check version */
	if ((in_len != sizeof(struct smi_xgmi_fb_sharing)) ||
	    (out_len != sizeof(struct smi_xgmi_fb_sharing_flag))) {
		return SMI_STATUS_INVAL;
	}

	xgmi = (struct smi_xgmi_fb_sharing_flag *) outb;
	id = (struct smi_xgmi_fb_sharing *) inb;

	src_adev = smi_get_handle(ctx, &id->src_dev, NULL, &dev_busy);
	if (!src_adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	dst_adev = smi_get_handle(ctx, &id->dst_dev, NULL, &dev_busy);
	if (!dst_adev) {
		smi_put_handle(src_adev, ctx);
		return SMI_STATUS_NOT_FOUND;
	}

	if (dev_busy) {
		smi_put_handle(src_adev, ctx);
		return SMI_STATUS_BUSY;
	}

	fb_sharing_mode = smi_map_fb_sharing_mode(id->mode);
	ret = amdgv_gpumon_get_xgmi_fb_sharing_mode_info(src_adev, dst_adev, fb_sharing_mode, &xgmi->fb_sharing);

	smi_put_handle(src_adev, ctx);
	smi_put_handle(dst_adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_set_xgmi_fb_sharing_mode(struct smi_ctx *ctx, void *inb,
			      void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_set_xgmi_fb_sharing_mode *id = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	enum amdgv_gpumon_xgmi_fb_sharing_mode fb_sharing_mode;
	int ret = SMI_STATUS_SUCCESS;

	/* Check version */
	if ((in_len != sizeof(struct smi_set_xgmi_fb_sharing_mode)) ||
	    (out_len != 0)) {
		return SMI_STATUS_INVAL;
	}

	id = (struct smi_set_xgmi_fb_sharing_mode *) inb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	fb_sharing_mode = smi_map_fb_sharing_mode(id->mode);
	ret = amdgv_gpumon_set_xgmi_fb_sharing_mode(adev, fb_sharing_mode);

	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_set_xgmi_fb_custom_sharing_mode(struct smi_ctx *ctx, void *inb,
			      void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_set_xgmi_fb_custom_sharing_mode *id = NULL;
	uint32_t dev_list_size;
	amdgv_dev_t *dev_list = NULL;
	bool dev_busy = false;
	int ret = SMI_STATUS_SUCCESS;
	uint32_t i = 0;
	int j = 0;

	/* Check version */
	if ((in_len != sizeof(struct smi_set_xgmi_fb_custom_sharing_mode)) ||
	    (out_len != 0)) {
		return SMI_STATUS_INVAL;
	}
	id = (struct smi_set_xgmi_fb_custom_sharing_mode *) inb;

	dev_list_size = id->num_processors;
	dev_list = smi_oss_funcs->alloc_memory(sizeof(amdgv_dev_t) * dev_list_size);

	if (dev_list == NULL) {
		return SMI_STATUS_OUT_OF_RESOURCES;
	}

	for (i = 0; i < dev_list_size; i++) {
		dev_list[i] = smi_get_handle(ctx, &id->dev_id_list[i], NULL, &dev_busy);
		if (dev_busy) {
			for (j = i - 1; j >= 0; j--) {
				smi_put_handle(dev_list[j], ctx);
			}
			smi_oss_funcs->free_memory(dev_list);
			return SMI_STATUS_BUSY;
		}
	}

	ret = amdgv_gpumon_set_xgmi_fb_custom_sharing_mode(dev_list_size, dev_list);

	for (i = 0; i < dev_list_size; i++) {
		smi_put_handle(dev_list[i], ctx);
	}

	smi_oss_funcs->free_memory(dev_list);
	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_ras_feature_info(struct smi_ctx *ctx, void *inb,
			     void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	struct smi_ras_feature *ras_info = NULL;
	int ret = SMI_STATUS_SUCCESS;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
	    (out_len != sizeof(struct smi_ras_feature))) {
		return SMI_STATUS_INVAL;
	}

	ras_info = (struct smi_ras_feature *) outb;
	id = (struct smi_device_info *) inb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	ras_info->ras_eeprom_version = 0;
	ret = amdgv_gpumon_get_ras_eeprom_version(adev, &ras_info->ras_eeprom_version);
	if (ret){
		goto end;
	}
	ret = amdgv_gpumon_get_ecc_correction_schema(adev, &ras_info->supported_ecc_correction_schema);
	if (ret == AMDGV_ERROR_GPUMON_NOT_SUPPORTED){
		ras_info->supported_ecc_correction_schema = SMI_NOT_SUPPORTED;
		ret = SMI_STATUS_SUCCESS;
	}
end:
	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_metrics_table(struct smi_ctx *ctx, void *inb,
			    void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_metrics_table *metrics_table = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	int ret = SMI_STATUS_SUCCESS;
	struct amdgv_gpumon_metrics_ext *gpumon_metrics_table = NULL;

	/* Check version */
	if ((in_len != sizeof(struct smi_metrics_table)) ||
	    (out_len != 0))
		return SMI_STATUS_INVAL;

	metrics_table = (struct smi_metrics_table *) inb;

	adev = smi_get_handle(ctx, &metrics_table->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	gpumon_metrics_table = smi_oss_funcs->alloc_memory(sizeof(struct amdgv_gpumon_metrics_ext));
	if (gpumon_metrics_table == NULL) {
		ret = SMI_STATUS_OUT_OF_RESOURCES;
		goto end;
	}

	ret = amdgv_gpumon_get_metrics_ext(adev, gpumon_metrics_table);
	if (ret)
		goto end;

	ret = smi_get_metric_table(metrics_table, in_len, gpumon_metrics_table);

end:
	smi_oss_funcs->free_memory(gpumon_metrics_table);
	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_memory_partition_config(struct smi_ctx *ctx, void *inb,
			  void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	struct smi_memory_partition_config *partition_setting = NULL;
	struct amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	struct amdgv_gpumon_memory_partition_info memory_partition_info;
	union amdgv_gpumon_memory_partition_config memory_partition_config;
	uint32_t i;
	int ret = SMI_STATUS_SUCCESS;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
	    (out_len != sizeof(struct smi_memory_partition_config))) {
		return SMI_STATUS_INVAL;
	}
	partition_setting = (struct smi_memory_partition_config *) outb;
	id = (struct smi_device_info *) inb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	/* Fill the data */
	ret = amdgv_gpumon_get_memory_partition_mode(adev, &memory_partition_info);
	if (ret)
		goto end;

	ret = amdgv_gpumon_get_memory_partition_config(adev, &memory_partition_config);
	if (ret)
		goto end;

	partition_setting->mp_mode = smi_map_mp_mode(memory_partition_info.memory_partition_mode);
	partition_setting->partition_caps.nps_cap_mask = memory_partition_config.mp_cap_mask;
	partition_setting->num_numa_ranges = memory_partition_info.num_numa_ranges;
	for (i = 0; i < partition_setting->num_numa_ranges; i++) {
		partition_setting->numa_range[i].memory_type = smi_map_vram_type(memory_partition_info.numa_range[i].memory_type);
		partition_setting->numa_range[i].start = memory_partition_info.numa_range[i].start;
		partition_setting->numa_range[i].end = memory_partition_info.numa_range[i].end;
	}

end:
	smi_put_handle(adev, ctx);
	return smi_convert_ret_value(ERROR_OTHER, ret);
}


int smi_set_memory_partition_setting(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_set_gpu_memory_partition_setting *id = NULL;
	struct amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	int ret = SMI_STATUS_SUCCESS;

	if ((in_len != sizeof(struct smi_set_gpu_memory_partition_setting)) ||
		(out_len != 0)) {
		return SMI_STATUS_INVAL;
	}

	id = (struct smi_set_gpu_memory_partition_setting *)inb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);

	if (!adev) {
		return SMI_STATUS_NOT_FOUND;
	}
	if (dev_busy)
		return SMI_STATUS_BUSY;

	ret = amdgv_gpumon_set_memory_partition_mode(adev, smi_map_memory_partition_mode(id->mode));

	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}


int smi_get_accelerator_partition_profile_config(struct smi_ctx *ctx, void *inb,
			  void *outb, uint16_t in_len, uint16_t out_len)
{

	struct smi_profile_configs *profile_configs = NULL;
	struct amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	struct amdgv_gpumon_accelerator_partition_profile_config *caps = NULL;
	int ret = SMI_STATUS_SUCCESS;

	/* Check version */
	if ((in_len != sizeof(struct smi_profile_configs)) ||
	    (out_len != 0)) {
		return SMI_STATUS_INVAL;
	}
	profile_configs = (struct smi_profile_configs *) inb;


	adev = smi_get_handle(ctx, &profile_configs->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	caps = smi_oss_funcs->alloc_memory(sizeof(struct amdgv_gpumon_accelerator_partition_profile_config));
	if (caps == NULL) {
		ret = SMI_STATUS_OUT_OF_RESOURCES;
		goto end;
	}

	/* Fill the data */
	ret = amdgv_gpumon_get_accelerator_partition_profile_config(adev, caps);
	if (ret)
		goto end;


	ret = smi_get_partition(profile_configs, caps);

end:
	smi_oss_funcs->free_memory(caps);
	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_accelerator_partition_profile(struct smi_ctx *ctx, void *inb,
			  void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_device_info *id = NULL;
	struct smi_accelerator_partition_profile_cap *config = NULL;
	struct amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	struct amdgv_gpumon_acccelerator_partition_profile profile;
	uint32_t i, j;
	int ret = SMI_STATUS_SUCCESS;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
	    (out_len != sizeof(struct smi_accelerator_partition_profile_cap))) {
		return SMI_STATUS_INVAL;
	}
	config = (struct smi_accelerator_partition_profile_cap *) outb;
	id = (struct smi_device_info *) inb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev)
		return SMI_STATUS_NOT_FOUND;
	if (dev_busy)
		return SMI_STATUS_BUSY;

	/* Fill the data */
	ret = amdgv_gpumon_get_accelerator_partition_profile(adev, &profile);
	if (ret) {
		goto end;
	}

	config->config.profile_type = smi_map_partition_type(profile.profile_type);
	config->config.num_partitions = profile.num_partitions;
	config->config.memory_caps.nps_cap_mask = profile.memory_caps.mp_cap_mask;
	config->config.profile_index = profile.profile_index;
	config->config.num_resources = profile.num_resources;
	for (i = 0; i < config->config.num_partitions; i++) {
		config->partition_id[i] = profile.partition_id[i];
		for (j = 0; j < config->config.num_resources; j++) {
			config->config.resources[i][j] = profile.resources[i][j];
		}
	}

end:
	smi_put_handle(adev, ctx);
	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_set_accelerator_partition_setting(struct smi_ctx *ctx, void *inb,
				void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_set_gpu_accelerator_partition_setting *id = NULL;
	struct amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	struct amdgv_gpumon_accelerator_partition_profile_config *caps = NULL;
	int ret = SMI_STATUS_SUCCESS;

	if ((in_len != sizeof(struct smi_set_gpu_accelerator_partition_setting)) ||
		(out_len != 0)) {
		return SMI_STATUS_INVAL;
	}

	id = (struct smi_set_gpu_accelerator_partition_setting *)inb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);

	if (!adev) {
		return SMI_STATUS_NOT_FOUND;
	}
	if (dev_busy)
		return SMI_STATUS_BUSY;

	caps = smi_oss_funcs->alloc_memory(sizeof(struct amdgv_gpumon_accelerator_partition_profile_config));
	if (caps == NULL) {
		ret = SMI_STATUS_OUT_OF_RESOURCES;
		goto end;
	}

	/* Fill the data */
	ret = amdgv_gpumon_get_accelerator_partition_profile_config(adev, caps);
	if (ret)
		goto end;

	// for example, if number_of_profiles is 2, valid indexes for setting are only 0 and 1
	if (id->index >= caps->number_of_profiles) {
		smi_oss_funcs->free_memory(caps);
		smi_put_handle(adev, ctx);
		return SMI_STATUS_INVAL;
	}

	ret = amdgv_gpumon_set_accelerator_partition_profile(adev, id->index);

end:
	smi_oss_funcs->free_memory(caps);
	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_soc_pstate(struct smi_ctx *ctx, void *inb,
			    void *outb, uint16_t in_len, uint16_t out_len)
{
	amdgv_dev_t *adev = NULL;
	struct smi_device_info *id = NULL;
	struct smi_dpm_policy *dpm_policy = NULL;
	struct amdgv_gpumon_smu_dpm_policy dpm_policy_info;
	bool dev_busy = false;
	int i;
	int ret = SMI_STATUS_SUCCESS;

	/* Check version */
	if ((in_len != sizeof(struct smi_device_info)) ||
	    (out_len != sizeof(struct smi_dpm_policy))) {
		return SMI_STATUS_INVAL;
	}

	dpm_policy = (struct smi_dpm_policy *) outb;
	id = (struct smi_device_info *) inb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev) {
		return SMI_STATUS_NOT_FOUND;
	}
	if (dev_busy)
		return SMI_STATUS_BUSY;

	ret = amdgv_gpumon_get_pm_policy(adev, AMDGV_PP_PM_POLICY_SOC_PSTATE, &dpm_policy_info);
	if (ret)
		goto end;

	dpm_policy->num_supported = AMDGV_GPUMON_SOC_PSTATE_COUNT;
	dpm_policy->cur = dpm_policy_info.current_level;

	for (i = 0; i < AMDGV_GPUMON_SOC_PSTATE_COUNT; i++) {
		dpm_policy->policies[i].policy_id = dpm_policy_info.policies[i].policy_id;
		smi_oss_funcs->memcpy(dpm_policy->policies[i].policy_description,
			dpm_policy_info.policies[i].policy_description,
			smi_oss_funcs->strlen(dpm_policy_info.policies[i].policy_description));
	}

end:
	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_set_soc_pstate(struct smi_ctx *ctx, void *inb,
			      void *outb, uint16_t in_len, uint16_t out_len)
{
	struct smi_set_dpm_policy *id = NULL;
	amdgv_dev_t *adev = NULL;
	bool dev_busy = false;
	int ret = SMI_STATUS_SUCCESS;

	/* Check version */
	if ((in_len != sizeof(struct smi_set_dpm_policy)) ||
	    (out_len != 0)) {
		return SMI_STATUS_INVAL;
	}

	id = (struct smi_set_dpm_policy *) inb;

	adev = smi_get_handle(ctx, &id->dev_id, NULL, &dev_busy);
	if (!adev) {
		return SMI_STATUS_NOT_FOUND;
	}
	if (dev_busy)
		return SMI_STATUS_BUSY;

	ret = amdgv_gpumon_set_pm_policy_level(adev, AMDGV_PP_PM_POLICY_SOC_PSTATE, id->policy_id);

	smi_put_handle(adev, ctx);

	return smi_convert_ret_value(ERROR_OTHER, ret);
}

int smi_get_cper_error(struct smi_ctx *ctx, void *inb,
			    void *outb, uint16_t in_len, uint16_t out_len)
{
	amdgv_dev_t *adev = NULL;
	struct smi_cper_config *config = NULL;
	bool dev_busy = false;
	int ret = SMI_STATUS_SUCCESS;

	uint32_t size = SMI_MAX_CPER_SIZE;
	uint64_t write_count = 0;
	uint64_t overflow_count = 0;
	uint64_t left_size = 0;
	char *buf = NULL;
	uint32_t smi_cper_hdrs[SMI_MAX_CPER_HDRS];

	struct smi_cper_hdr * hdr = NULL;
	unsigned int i = 0;

	/* Check version */
	if ((in_len != sizeof(struct smi_cper_config)) ||
		(out_len != 0)) {
		return SMI_STATUS_INVAL;
	}

	buf = smi_oss_funcs->alloc_memory(size);
	if (buf == NULL) {
		return SMI_STATUS_OUT_OF_RESOURCES;
	}

	config = (struct smi_cper_config *) inb;

	adev = smi_get_handle(ctx, &config->dev_id, NULL, &dev_busy);
	if (!adev) {
		smi_oss_funcs->free_memory(buf);
		return SMI_STATUS_NOT_FOUND;
	}
	if (dev_busy){
		smi_oss_funcs->free_memory(buf);
		return SMI_STATUS_BUSY;
	}

	ret = amdgv_gpumon_cper_get_entries(adev, config->input_cursor, buf, size, &write_count, &overflow_count, &left_size);
	if (ret) {
		goto end;
	}

	hdr = (struct smi_cper_hdr*)(buf);
	smi_cper_hdrs[0] = 0;

	write_count = write_count >= SMI_MAX_CPER_HDRS ? SMI_MAX_CPER_HDRS : write_count;

	for (i = 1; i < write_count; i++) {
		smi_cper_hdrs[i] = smi_cper_hdrs[i-1] + hdr->record_length;

		/* next entry */
		hdr = (struct smi_cper_hdr*)((char *)hdr + hdr->record_length);
	}

	ret = smi_get_cper_data(config, in_len, size, buf, write_count, smi_cper_hdrs);
	if (ret)
		goto end;

	if (left_size != 0) {
		smi_oss_funcs->free_memory(buf);
		smi_put_handle(adev, ctx);
		return SMI_STATUS_MORE_DATA;
	}

end:
	smi_oss_funcs->free_memory(buf);
	smi_put_handle(adev, ctx);
	return smi_convert_ret_value(ERROR_OTHER, ret);
}
