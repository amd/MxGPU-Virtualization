/*
 * Copyright (c) 2017-2022 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv_device.h"
#include "amdgv_gpumon.h"
#include "amdgv_gpumon_internal.h"
#include "amdgv_api_internal.h"
#include "amdgv_sched_internal.h"
#include "amdgv_vfmgr.h"
#include "amdgv_psp_gfx_if.h"
#include "atombios/atomfirmware.h"
#include "atombios/atom.h"
#include "atombios/atombios.h"
#include "amdgv_xgmi.h"

static const char *amdgv_gpumon_event_name(enum amdgv_gpumon_type type)
{
	switch (type) {
	case GPUMON_GET_ASIC_TEMP:
		return "GPUMON_GET_ASIC_TEMP";
	case GPUMON_GET_ALL_TEMP:
		return "GPUMON_GET_ALL_TEMP";
	case GPUMON_GET_GPU_POWER_USAGE:
		return "GPUMON_GET_GPU_POWER_USAGE";
	case GPUMON_GET_GPU_POWER_CAP:
		return "GPUMON_GET_GPU_POWER_CAP";
	case GPUMON_SET_GPU_POWER_CAP:
		return "GPUMON_SET_GPU_POWER_CAP";
	case GPUMON_GET_VDDC:
		return "GPUMON_GET_VDDC";
	case GPUMON_GET_DPM_STATUS:
		return "GPUMON_GET_DPM_STATUS";
	case GPUMON_GET_PCIE_CONFS:
		return "GPUMON_GET_PCIE_CONFS";
	case GPUMON_GET_DPM_CAP:
		return "GPUMON_GET_DPM_CAP";
	case GPUMON_GET_GFX_ACT:
		return "GPUMON_GET_GFX_ACT";
	case GPUMON_GET_MEM_ACT:
		return "GPUMON_GET_MEM_ACT";
	case GPUMON_GET_UVD_ACT:
		return "GPUMON_GET_UVD_ACT";
	case GPUMON_GET_VCE_ACT:
		return "GPUMON_GET_VCE_ACT";
	case GPUMON_GET_ECC_INFO:
		return "GPUMON_GET_ECC_INFO";
	case GPUMON_GET_PP_METRICS:
		return "GPUMON_GET_PP_METRICS";
	case GPUMON_GET_SCLK:
		return "GPUMON_GET_SCLK";
	case GPUMON_GET_MAX_SCLK:
		return "GPUMON_GET_MAX_SCLK";
	case GPUMON_GET_MAX_MCLK:
		return "GPUMON_GET_MAX_MCLK";
	case GPUMON_GET_MAX_VCLK0:
		return "GPUMON_GET_MAX_VCLK0";
	case GPUMON_GET_MAX_VCLK1:
		return "GPUMON_GET_MAX_VCLK1";
	case GPUMON_GET_MAX_DCLK0:
		return "GPUMON_GET_MAX_DCLK0";
	case GPUMON_GET_MAX_DCLK1:
		return "GPUMON_GET_MAX_DCLK1";
	case GPUMON_GET_MIN_SCLK:
		return "GPUMON_GET_MIN_SCLK";
	case GPUMON_GET_MIN_MCLK:
		return "GPUMON_GET_MIN_MCLK";
	case GPUMON_GET_MIN_VCLK0:
		return "GPUMON_GET_MIN_VCLK0";
	case GPUMON_GET_MIN_VCLK1:
		return "GPUMON_GET_MIN_VCLK1";
	case GPUMON_GET_MIN_DCLK0:
		return "GPUMON_GET_MIN_DCLK0";
	case GPUMON_GET_MIN_DCLK1:
		return "GPUMON_GET_MIN_DCLK1";
	case GPUMON_FRU_SIGOUT:
		return "GPUMON_FRU_SIGOUT";
	case GPUMON_RAS_EEPROM_CLEAR:
		return "GPUMON_RAS_EEPROM_CLEAR";
	case GPUMON_RAS_ERROR_INJECT:
		return "GPUMON_RAS_ERROR_INJECT";
	case GPUMON_TURN_ON_ECC_INJECTION:
		return "GPUMON_TURN_ON_ECC_INJECTION";
	case GPUMON_ALLOC_VF:
		return "GPUMON_ALLOC_VF";
	case GPUMON_SET_VF_NUM:
		return "GPUMON_SET_VF_NUM";
	case GPUMON_FREE_VF:
		return "GPUMON_FREE_VF";
	case GPUMON_SET_VF:
		return "GPUMON_SET_VF";
	case GPUMON_GET_FW_ERR_RECORDS:
		return "GPUMON_GET_FW_ERR_RECORDS";
	case GPUMON_GET_VF_FW_INFO:
		return "GPUMON_GET_VF_FW_INFO";
	case GPUMON_CLEAN_CORRECTABLE_ERROR_COUNT:
		return "GPUMON_CLEAN_CORRECTABLE_ERROR_COUNT";
	case GPUMON_GET_VRAM_INFO:
		return "GPUMON_GET_VRAM_INFO";
	case GPUMON_IS_CLK_LOCKED:
		return "GPUMON_IS_CLK_LOCKED";
	case GPUMON_SET_ACCELERATOR_PARTITION_PROFILE:
		return "GPUMON_SET_ACCELERATOR_PARTITION_PROFILE";
	case GPUMON_SET_MEMORY_PARTITION_MODE:
		return "GPUMON_SET_MEMORY_PARTITION_MODE";
	case GPUMON_SET_SPATIAL_PARTITION_NUM:
		return "GPUMON_SET_SPATIAL_PARTITION_NUM";
	case GPUMON_RESET_SPATIAL_PARTITION_NUM:
		return "GPUMON_RESET_SPATIAL_PARTITION_NUM";
	case GPUMON_GET_ACCELERATOR_PARTITION_PROFILE:
		return "GPUMON_GET_ACCELERATOR_PARTITION_PROFILE";
	case GPUMON_GET_MEMORY_PARTITION_MODE:
		return "GPUMON_GET_MEMORY_PARTITION_MODE";
	case GPUMON_GET_SPATIAL_PARTITION_NUM:
		return "GPUMON_GET_SPATIAL_PARTITION_NUM";
	case GPUMON_GET_PCIE_REPLAY_COUNT:
		return "GPUMON_GET_PCIE_REPLAY_COUNT";
	case GPUMON_GET_CARD_FORM_FACTOR:
		return "GPUMON_GET_CARD_FORM_FACTOR";
	case GPUMON_GET_MAX_CONFIG_POWER_LIMIT:
		return "GPUMON_GET_MAX_CONFIG_POWER_LIMIT";
	case GPUMON_GET_DEFAULT_POWER_LIMIT:
		return "GPUMON_GET_DEFAULT_POWER_LIMIT";
	case GPUMON_GET_MIN_POWER_LIMIT:
		return "GPUMON_GET_MIN_POWER_LIMIT";
	case GPUMON_GET_METRICS_EXT:
		return "GPUMON_GET_METRICS_EXT";
	case GPUMON_GET_NUM_METRICS_EXT_ENTRIES:
		return "GPUMON_GET_NUM_METRICS_EXT_ENTRIES";
	case GPUMON_IS_POWER_MANAGEMENT_ENABLED:
		return "GPUMON_IS_POWER_MANAGEMENT_ENABLED";
	case GPUMON_GET_LINK_METRICS:
		return "GPUMON_GET_LINK_METRICS";
	case GPUMON_GET_LINK_TOPOLOGY:
		return "GPUMON_GET_LINK_TOPOLOGY";
	case GPUMON_GET_XGMI_FB_SHARING_CAPS:
		return "GPUMON_GET_XGMI_FB_SHARING_CAPS";
	case GPUMON_GET_XGMI_FB_SHARING_MODE_INFO:
		return "GPUMON_GET_XGMI_FB_SHARING_MODE_INFO";
	case GPUMON_SET_XGMI_FB_SHARING_MODE:
		return "GPUMON_SET_XGMI_FB_SHARING_MODE";
	case GPUMON_SET_XGMI_FB_SHARING_MODE_EX:
		return "GPUMON_SET_XGMI_FB_SHARING_MODE_EX";
	case GPUMON_GET_SHUTDOWN_TEMP:
		return "GPUMON_GET_SHUTDOWN_TEMP";
	case GPUMON_GET_GPU_CACHE_INFO:
		return "GPUMON_GET_GPU_CACHE_INFO";
	case GPUMON_GET_MAX_PCIE_LINK_GENERATION:
		return "GPUMON_GET_MAX_PCIE_LINK_GENERATION";
	case GPUMON_RAS_REPORT:
		return "GPUMON_RAS_REPORT";
	case GPUMON_RAS_TA_LOAD:
		return "GPUMON_RAS_TA_LOAD";
	case GPUMON_RAS_TA_UNLOAD:
		return "GPUMON_RAS_TA_UNLOAD";
	case GPUMON_RESET_ALL_ERROR_COUNTS:
		return "GPUMON_RESET_ALL_ERROR_COUNTS";
	case GPUMON_GET_FRU_PRODUCT_INFO:
		return "GPUMON_GET_FRU_PRODUCT_INFO";
	case GPUMON_SET_PM_POLICY_LEVEL:
		return "GPUMON_SET_PM_POLICY_LEVEL";
	case GPUMON_GET_DPM_POLICY_LEVEL:
		return "GPUMON_GET_DPM_POLICY_LEVEL";
	case GPUMON_GET_XGMI_PLPD_POLICY_LEVEL:
		return "GPUMON_GET_XGMI_PLPD_POLICY_LEVEL";
	case GPUMON_CPER_GET_COUNT:
		return "GPUMON_CPER_GET_COUNT";
	case GPUMON_CPER_GET_ENTRIES:
		return "GPUMON_CPER_GET_ENTRIES";
	case GPUMON_GET_GFX_CONFIG:
		return "GPUMON_GET_GFX_CONFIG";
	default:
		break;
	}
	return "UNKNOWN";
}
static const uint32_t this_block = AMDGV_MANAGEMENT_BLOCK;

/* 1000 ms */
#define MAX_TIME_SLICE		   (1000 * 1000)
#define PP_METRICS_CACHE_EXPIRY_US 1000

static bool amdgv_find_fb_offset(amdgv_dev_t dev, struct amdgv_vf_option *opt)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	struct amdgv_vf_device *vf;
	int i;

	// start and end fb location
	uint32_t opt_s, opt_e;
	uint32_t vf_s, vf_e;

	uint32_t max;
	uint32_t min;

	opt_s = adapt->array_vf[AMDGV_PF_IDX].fb_size +
		adapt->array_vf[AMDGV_PF_IDX].fb_offset;

	opt_e = opt_s + opt->fb_size;

	if (opt_e > adapt->gpuiov.total_fb_avail)
		return true;

	// Check all configured functions (vf & pf)
	for (i = 0; i < adapt->num_vf; i++) {
		vf = &adapt->array_vf[i];

		if (vf->fb_size == 0 || vf->idx_vf == opt->idx_vf || !vf->configured)
			continue;

		vf_s = vf->fb_offset;
		vf_e = vf->fb_offset + vf->fb_size;

		max = opt_e > vf_e ? opt_e : vf_e;
		min = opt_s < vf_s ? opt_s : vf_s;

		if (max - min < opt->fb_size + vf->fb_size) {
			opt_s = vf_e;
			opt_e = opt_s + opt->fb_size;
			if (opt_e > adapt->gpuiov.total_fb_avail)
				return true;
		}
	}

	opt->fb_offset = opt_s;

	return false;
}

/* For backwards compatibility we need to ignore the incoming
 * opt type
 */
int amdgv_vf_get_option_type(struct amdgv_adapter *adapt, enum amdgv_set_vf_opt_type *opt_type,
			     struct amdgv_vf_option *opt)
{
	struct amdgv_vf_device *vf;

	vf = &adapt->array_vf[opt->idx_vf];
	if (*opt_type & AMDGV_SET_VF_FB) {
		if ((opt->fb_size == 0) && (opt->fb_offset == 0))
			*opt_type &= ~AMDGV_SET_VF_FB;
		if ((opt->fb_size == vf->fb_size_os) && (opt->fb_offset == vf->fb_offset_os))
			*opt_type &= ~AMDGV_SET_VF_FB;
	}

	return 0;
}

int amdgv_vf_option_valid(amdgv_dev_t dev, enum amdgv_set_vf_opt_type opt_type,
			  struct amdgv_vf_option *opt)
{
	int ret = 0;
	struct amdgv_adapter *adapt;
	uint32_t time_slice;
	int i;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	/* check idx_vf */
	if (AMDGV_IS_IDX_INVALID(opt->idx_vf) && (opt->idx_vf != -1)) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_GPUMON_INVALID_VF_INDEX,
				opt->idx_vf);
		ret = AMDGV_ERROR_GPUMON_INVALID_VF_INDEX;
		goto end;
	}

	if (opt_type & AMDGV_SET_VF_FB) {
		/* check fb size */
		if ((opt->fb_size % 16 != 0) || (opt->fb_size < 256)) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_GPUMON_INVALID_FB_SIZE,
					opt->fb_size);
			ret = AMDGV_ERROR_GPUMON_INVALID_FB_SIZE;
			goto end;
		}

		if (opt->fb_offset == 0) {
			if (amdgv_find_fb_offset(dev, opt)) {
				amdgv_put_error(AMDGV_PF_IDX,
						AMDGV_ERROR_GPUMON_NO_SUITABLE_SPACE,
						opt->fb_size);
				ret = AMDGV_ERROR_GPUMON_NO_SUITABLE_SPACE;
				goto end;
			}
		}

		/* check whether new fb is oversize */
		if (opt->fb_offset + opt->fb_size > adapt->gpuiov.total_fb_usable) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_GPUMON_OVERSIZE_ALLOCATION,
					AMDGV_ERROR_32_32(opt->fb_offset, opt->fb_size));
			ret = AMDGV_ERROR_GPUMON_OVERSIZE_ALLOCATION;
			goto end;
		}

		/* check fb overlapping */
		if (amdgv_is_fb_overlap(dev, opt)) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_GPUMON_OVERLAPPING_FB,
					AMDGV_ERROR_32_32(opt->fb_offset,
							  opt->fb_offset + opt->fb_size));
			ret = AMDGV_ERROR_GPUMON_OVERLAPPING_FB;
			goto end;
		}
	}

	if (opt_type & AMDGV_SET_VF_TIME) {
		/* check graphic time slice */
		if (opt->gfx_time_slice > MAX_TIME_SLICE &&
		    opt->gfx_time_slice != DEFAULT_GFX_TIME_SLICE_1VF) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_GPUMON_INVALID_GFX_TIMESLICE,
					AMDGV_ERROR_32_32(opt->gfx_time_slice,
							  MAX_TIME_SLICE));
			ret = AMDGV_ERROR_GPUMON_INVALID_GFX_TIMESLICE;
			goto end;
		}

		/* check mm time slice */
		for (i = 0; i < AMDGV_MAX_MM_ENGINE; i++) {
			time_slice = amdgv_sched_bandwidth_to_time_slice(adapt, i,
									 opt->mm_bandwidth[i]);
			if (time_slice > MAX_TIME_SLICE) {
				amdgv_put_error(AMDGV_PF_IDX,
						AMDGV_ERROR_GPUMON_INVALID_MM_TIMESLICE,
						AMDGV_ERROR_32_32(time_slice, MAX_TIME_SLICE));
				ret = AMDGV_ERROR_GPUMON_INVALID_MM_TIMESLICE;
				goto end;
			}
		}
	}

	if (opt_type & AMDGV_SET_VF_GFX_PART) {
		if (!amdgv_get_vf_gfx_part_valid(dev, opt)) {
			amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_GPUMON_INVALID_GFX_PART, 0);
			ret = AMDGV_ERROR_GPUMON_INVALID_GFX_PART;
			goto end;
		}
	}

end:
	return ret;
}

bool amdgv_get_vf_gfx_part_valid(amdgv_dev_t dev, struct amdgv_vf_option *opt)
{
	uint32_t total_time_slice;
	uint32_t time_slice;
	uint32_t used_time_slice = 0;
	struct amdgv_adapter *adapt;
	struct amdgv_vf_device *vf;
	int i;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if ((opt->gfx_time_divider < 1) || (opt->gfx_time_divider > AMDGV_MAX_VF_SLOT))
		return false;

	/* Only accept divider 1, 2, 4 ..., 32. */
	if ((opt->gfx_time_divider & (opt->gfx_time_divider - 1)) != 0)
		return false;

	if (adapt->sched.num_vf_per_gfx_sched == 1)
		total_time_slice = DEFAULT_GFX_TIME_SLICE_1VF;
	else
		total_time_slice =
			adapt->num_vf *
			GET_GFX_TIME_SLICE(adapt, adapt->sched.num_vf_per_gfx_sched);

	for (i = 0; i < adapt->num_vf; i++) {
		vf = &adapt->array_vf[i];
		if (vf->configured && (i != opt->idx_vf))
			used_time_slice += vf->time_slice[AMDGV_SCHED_BLOCK_GFX];
	}

	time_slice = total_time_slice / opt->gfx_time_divider;

	/* checking if time slice is over assigned */
	if (total_time_slice < (used_time_slice + time_slice))
		return false;

	opt->gfx_time_slice = time_slice;
	return true;
}

bool amdgv_is_fb_overlap(amdgv_dev_t dev, struct amdgv_vf_option *opt)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;
	struct amdgv_vf_device *vf;
	int i;

	// start and end fb location
	uint32_t opt_s, opt_e;
	uint32_t vf_s, vf_e;

	uint32_t max;
	uint32_t min;

	opt_s = opt->fb_offset;
	opt_e = opt->fb_offset + opt->fb_size;

	// Check all configured functions (vf & pf)
	for (i = 0; i < adapt->num_vf; i++) {
		vf = &adapt->array_vf[i];

		if (vf->fb_size_os == 0 || vf->idx_vf == opt->idx_vf || !vf->configured)
			continue;

		vf_s = vf->fb_offset_os;
		vf_e = vf->fb_offset_os + vf->fb_size_os;

		max = opt_e > vf_e ? opt_e : vf_e;
		min = opt_s < vf_s ? opt_s : vf_s;

		if (max - min < opt->fb_size + vf->fb_size_os)
			return true;
	}

	// Check PF
	vf = &adapt->array_vf[AMDGV_PF_IDX];

	vf_s = vf->fb_offset;
	vf_e = vf->fb_offset + vf->fb_size;

	max = opt_e > vf_e ? opt_e : vf_e;
	min = opt_s < vf_s ? opt_s : vf_s;

	if (max - min < opt->fb_size + vf->fb_size)
		return true;

	return false;
}

uint32_t amdgv_parse_divider(uint32_t divider)
{
	unsigned int divisor;

	if (divider < 8)
		divisor = 0;
	else if (divider <= 0x3F)
		divisor = divider * 25;
	else if (divider <= 0x5F)
		divisor = ((divider - 0x40) * 50) + 1600;
	else if (divider <= 0x7E)
		divisor = ((divider - 0x60) * 100) + 3200;
	else if (divider == 0x7F)
		divisor = 12800;
	else
		divisor = 0;

	return divisor;
}

int amdgv_gpumon_get_gpu_power_usage(amdgv_dev_t dev, int *val)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!val)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = val;
	data.gpumon_data.type = GPUMON_GET_GPU_POWER_USAGE;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_gpu_power_usage && val) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_gpu_power_capacity(amdgv_dev_t dev, int *val)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!val)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = val;
	data.gpumon_data.type = GPUMON_GET_GPU_POWER_CAP;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_gpu_power_capacity && val) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_set_gpu_power_capacity(amdgv_dev_t dev, int val)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	data.gpumon_data.val = val;
	data.gpumon_data.type = GPUMON_SET_GPU_POWER_CAP;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->set_gpu_power_capacity) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_asic_temperature(amdgv_dev_t dev, int *val)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!val)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = val;
	data.gpumon_data.type = GPUMON_GET_ASIC_TEMP;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_asic_temperature && val) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_dpm_status(amdgv_dev_t dev, int *val)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!val)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = val;
	data.gpumon_data.type = GPUMON_GET_DPM_STATUS;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_dpm_status && val) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_dpm_cap(amdgv_dev_t dev, int *val)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!val)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = val;
	data.gpumon_data.type = GPUMON_GET_DPM_CAP;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_dpm_cap && val) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_pcie_confs(amdgv_dev_t dev, struct gim_dev_data *dev_data,
				int (*callback)(struct gim_dev_data *, int *, int *, int *),
				int *speed, int *width, int *max_vf)
{
	int event_ret = 0;
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data = { 0 };
	int ret;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);
	data.gpumon_data.pcie_confs.cb = callback;
	data.gpumon_data.pcie_confs.dev = (void *)dev_data;
	data.gpumon_data.pcie_confs.output.speed = speed;
	data.gpumon_data.pcie_confs.output.width = width;
	data.gpumon_data.pcie_confs.output.max_vf = max_vf;
	data.gpumon_data.result = &event_ret;
	data.gpumon_data.type = GPUMON_GET_PCIE_CONFS;

	ret = amdgv_sched_queue_event_and_wait_ex(
		adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_GPUMON, AMDGV_SCHED_BLOCK_ALL, data);

	/* data are collected */
	if (!ret)
		ret = event_ret;

	return ret;
}

int amdgv_gpumon_get_ecc_enabled(amdgv_dev_t dev, bool *val)
{
	struct amdgv_adapter *adapt;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	uint32_t enable = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!val)
		return AMDGV_FAILURE;

	*val = false;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_gecc)
		ret = adapt->gpumon.funcs->get_gecc(adapt, &enable);

	if (enable)
		*val = true;

	return ret;
}

int amdgv_gpumon_get_ecc_support_flag(amdgv_dev_t dev, uint32_t *supported, uint32_t *enabled)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (enabled)
		*enabled = adapt->ecc.enabled;

	if (supported)
		*supported = adapt->ecc.supported;

	return 0;
}

int amdgv_gpumon_get_ecc_correction_schema(amdgv_dev_t dev, uint32_t *ecc_correction_schema)
{
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;

	struct amdgv_adapter *adapt;

	if (ecc_correction_schema)
		*ecc_correction_schema = 0;
	else
		return AMDGV_FAILURE;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	/* Temporary method to retrive supported RAS ECC correction schema for MI300
	@TODO: Change this method when MI300 ECC changes are ready.
	set the supported ECC correction schema during ECC hw init*/
	if ((adapt->asic_type == CHIP_MI300X) || (adapt->asic_type == CHIP_MI308X)) {
		*ecc_correction_schema |=
			(1 << AMDGV_RAS_ECC_SUPPORT_PARITY) |
			(1 << AMDGV_RAS_ECC_SUPPORT_CORRECTABLE) |
			(1 << AMDGV_RAS_ECC_SUPPORT_UNCORRECTABLE) |
			(1 << AMDGV_RAS_ECC_SUPPORT_POISON);
		ret = 0;
	}

	return ret;
}

int amdgv_gpumon_get_metrics(amdgv_dev_t dev, struct amdgv_gpumon_metrics *metrics)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!metrics)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = metrics;
	data.gpumon_data.type = GPUMON_GET_PP_METRICS;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_pp_metrics && metrics) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_max_sclk(amdgv_dev_t dev, int *val)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!val)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = val;
	data.gpumon_data.type = GPUMON_GET_MAX_SCLK;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_max_sclk && val) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_max_mclk(amdgv_dev_t dev, int *val)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!val)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = val;
	data.gpumon_data.type = GPUMON_GET_MAX_MCLK;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_max_mclk) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_max_vclk0(amdgv_dev_t dev, int *val)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!val)
		return AMDGV_FAILURE;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_max_vclk0) {
		data.gpumon_data.ptr = val;
		data.gpumon_data.type = GPUMON_GET_MAX_VCLK0;
		data.gpumon_data.result = &event_ret;

		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_max_vclk1(amdgv_dev_t dev, int *val)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!val)
		return AMDGV_FAILURE;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_max_vclk1) {
		data.gpumon_data.ptr = val;
		data.gpumon_data.type = GPUMON_GET_MAX_VCLK1;
		data.gpumon_data.result = &event_ret;

		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_max_dclk0(amdgv_dev_t dev, int *val)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!val)
		return AMDGV_FAILURE;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_max_dclk0) {
		data.gpumon_data.ptr = val;
		data.gpumon_data.type = GPUMON_GET_MAX_DCLK0;
		data.gpumon_data.result = &event_ret;

		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_max_dclk1(amdgv_dev_t dev, int *val)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!val)
		return AMDGV_FAILURE;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_max_dclk1) {
		data.gpumon_data.ptr = val;
		data.gpumon_data.type = GPUMON_GET_MAX_DCLK1;
		data.gpumon_data.result = &event_ret;

		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_min_sclk(amdgv_dev_t dev, int *val)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!val)
		return AMDGV_FAILURE;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_min_sclk) {
		data.gpumon_data.ptr = val;
		data.gpumon_data.type = GPUMON_GET_MIN_SCLK;
		data.gpumon_data.result = &event_ret;

		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
						  AMDGV_EVENT_SCHED_GPUMON,
						  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_min_mclk(amdgv_dev_t dev, int *val)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!val)
		return AMDGV_FAILURE;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_min_mclk) {
		data.gpumon_data.ptr = val;
		data.gpumon_data.type = GPUMON_GET_MIN_MCLK;
		data.gpumon_data.result = &event_ret;

		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
						  AMDGV_EVENT_SCHED_GPUMON,
						  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_min_vclk0(amdgv_dev_t dev, int *val)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!val)
		return AMDGV_FAILURE;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_min_vclk0) {
		data.gpumon_data.ptr = val;
		data.gpumon_data.type = GPUMON_GET_MIN_VCLK0;
		data.gpumon_data.result = &event_ret;

		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
						  AMDGV_EVENT_SCHED_GPUMON,
						  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_min_vclk1(amdgv_dev_t dev, int *val)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!val)
		return AMDGV_FAILURE;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_min_vclk1) {
		data.gpumon_data.ptr = val;
		data.gpumon_data.type = GPUMON_GET_MIN_VCLK1;
		data.gpumon_data.result = &event_ret;

		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
						  AMDGV_EVENT_SCHED_GPUMON,
						  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_min_dclk0(amdgv_dev_t dev, int *val)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!val)
		return AMDGV_FAILURE;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_min_dclk0) {
		data.gpumon_data.ptr = val;
		data.gpumon_data.type = GPUMON_GET_MIN_DCLK0;
		data.gpumon_data.result = &event_ret;

		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
						  AMDGV_EVENT_SCHED_GPUMON,
						  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_min_dclk1(amdgv_dev_t dev, int *val)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!val)
		return AMDGV_FAILURE;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_min_dclk1) {
		data.gpumon_data.ptr = val;
		data.gpumon_data.type = GPUMON_GET_MIN_DCLK1;
		data.gpumon_data.result = &event_ret;

		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
						  AMDGV_EVENT_SCHED_GPUMON,
						  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

/* clear all ras bad page records stored in eeprom */
int amdgv_gpumon_ras_eeprom_clear(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS_NOT_LOST(adapt, dev);

	if (!adapt->umc.funcs)
		return AMDGV_FAILURE;

	amdgv_device_set_status(adapt, AMDGV_STATUS_HW_INIT);

	data.gpumon_data.type = GPUMON_RAS_EEPROM_CLEAR;
	data.gpumon_data.result = &event_ret;

	ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							AMDGV_EVENT_SCHED_GPUMON,
							AMDGV_SCHED_BLOCK_ALL, data);
	if (!ret)
		ret = event_ret;
	else
		amdgv_device_set_status(adapt, AMDGV_STATUS_HW_RMA);

	return ret;
}

/* return ue count recorded in eh_data(cached bad page arrary) */
void amdgv_gpumon_get_bad_page_record_count(amdgv_dev_t dev, int *bp_cnt)
{
	struct amdgv_adapter *adapt;

	*bp_cnt = 0;

	if (dev == AMDGV_INVALID_HANDLE)
		return;

	adapt = (struct amdgv_adapter *)dev;
	if (adapt->status == AMDGV_STATUS_HW_INIT || adapt->status == AMDGV_STATUS_HW_RMA)
		amdgv_umc_badpages_count_read(adapt, bp_cnt);
}

/* return one bad page record at one time according to index */
int amdgv_gpumon_get_bad_page_info(amdgv_dev_t dev, uint32_t index,
				   struct amdgv_smi_ras_eeprom_table_record *record)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS_NOT_LOST(adapt, dev);

	return amdgv_umc_get_badpages_record(adapt, index, (void *)record);
}

/* Note: the caller should free bp (array length is count) */
int amdgv_gpumon_ras_get_badpages_info(amdgv_dev_t dev, struct amdgv_smi_ras_badpage **bp,
				       unsigned int *count)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS_NOT_LOST(adapt, dev);

	return amdgv_umc_badpages_read(adapt, (void **)bp, count);
}

int amdgv_gpumon_get_ras_eeprom_version(amdgv_dev_t dev, uint32_t *ras_eeprom_version)
{
	struct amdgv_adapter *adapt;
	struct amdgv_ras_eeprom_control *control;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;

	SET_ADAPT_AND_CHECK_STATUS_NOT_LOST(adapt, dev);

	if (!ras_eeprom_version)
		return AMDGV_FAILURE;

	control = &adapt->eeprom_control;
	if (control->tbl_hdr.version) {
		*ras_eeprom_version = control->tbl_hdr.version;
		ret = 0;
	} else {
		*ras_eeprom_version = 0;
	}

	return ret;
}

int amdgv_gpumon_ras_error_inject(amdgv_dev_t dev,
				  struct amdgv_smi_ras_error_inject_info *data)
{
	return amdgv_ras_trigger_error(dev, data);
}

int amdgv_gpumon_ras_ta_load(amdgv_dev_t dev,
				  struct amdgv_smi_cmd_ras_ta_load *data)
{
	return amdgv_ras_ta_load(dev, data);
}

int amdgv_gpumon_ras_ta_unload(amdgv_dev_t dev,
					struct amdgv_smi_cmd_ras_ta_unload *data)
{
	return amdgv_ras_ta_unload(dev, data);
}

int amdgv_gpumon_turn_on_ecc_injection(amdgv_dev_t dev, const uint8_t *passphrase)
{
	int ret;
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	data.gpumon_data.type = GPUMON_TURN_ON_ECC_INJECTION;
	data.gpumon_data.ptr = (void *)passphrase;
	data.gpumon_data.result = &event_ret;

	ret = amdgv_sched_queue_event_and_wait_ex(
		adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_GPUMON, AMDGV_SCHED_BLOCK_ALL, data);
	if (!ret)
		ret = event_ret;

	return ret;
}

int amdgv_gpumon_get_ecc_info(amdgv_dev_t dev, struct amdgv_smi_ras_query_if *info)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!info)
		return AMDGV_FAILURE;

	if (adapt->ecc.get_error_count) {
		data.gpumon_data.type = GPUMON_GET_ECC_INFO;
		data.gpumon_data.ptr = info;
		data.gpumon_data.result = &event_ret;

		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_clean_correctable_error_count(amdgv_dev_t dev, int *corr)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;
	struct amdgv_smi_ras_query_if info = { 0 };

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!corr)
		return AMDGV_FAILURE;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_ecc_info &&
	    adapt->gpumon.funcs->clean_correctable_error_count) {
		data.gpumon_data.type = GPUMON_CLEAN_CORRECTABLE_ERROR_COUNT;
		data.gpumon_data.ptr = &info;
		data.gpumon_data.result = &event_ret;

		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	*corr = info.ce_count;

	return ret;
}

int amdgv_gpumon_ras_report(amdgv_dev_t dev, int ras_type)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->ras_report) {
		data.gpumon_data.type = GPUMON_RAS_REPORT;
		data.gpumon_data.val = ras_type;
		data.gpumon_data.result = &event_ret;

		// There is no need to wait for this event,
		// since we will execute reset first
		ret = amdgv_sched_queue_event_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_asic_serial(amdgv_dev_t dev, uint64_t *serial)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (adapt->serial) {
		*serial = adapt->serial;
		return 0;
	}

	return AMDGV_FAILURE;
}
int amdgv_gpumon_get_asic_type(amdgv_dev_t dev, uint32_t *asic_type)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (asic_type) {
		*asic_type = adapt->asic_type;
		return 0;
	}

	return AMDGV_FAILURE;
}

int amdgv_gpumon_get_dev_uuid(amdgv_dev_t dev, uint64_t *uuid)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS_NOT_LOST(adapt, dev);

	if (uuid && !amdgv_vfmgr_get_adapt_uuid(adapt, uuid))
		return 0;

	return AMDGV_FAILURE;
}

int amdgv_gpumon_get_vbios_info(amdgv_dev_t dev, struct amdgv_vbios_info *vbiosinfo)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	oss_memcpy(vbiosinfo, &adapt->vbios_cache, sizeof(*vbiosinfo));

	return 0;
}

/* Return types
 *
 * AMDGV_FAILURE => UNSUPPORTED
 * AMDGV_ERROR_DRIVER_GET_READ_SEMA_FAIL => RETRY
 * AMDGV_ERROR_DRIVER_INVALID_VALUE => INVALID
 * AMDGV_ERROR_DRIVER_DEV_INIT_FAIL => DEVICE NOT INITIALIZED
 */

int amdgv_gpumon_clear_vf_fb(amdgv_dev_t dev, uint32_t idx_vf, uint8_t pattern)
{
	int ret;
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	data.vf_fb_data.pattern = pattern;
	data.vf_fb_data.flag = 1;
	data.vf_fb_data.result = &event_ret;

	if (AMDGV_IS_IDX_INVALID(idx_vf) || idx_vf == AMDGV_PF_IDX)
		return AMDGV_ERROR_GPUMON_INVALID_VF_INDEX;

	if (!is_avail_vf(idx_vf) || is_vf_in_full_access(idx_vf))
		return AMDGV_ERROR_GPUMON_VF_BUSY;

	ret = amdgv_sched_queue_event_and_wait_ex(adapt, idx_vf, AMDGV_EVENT_SCHED_INIT_VF_FB,
						  AMDGV_SCHED_BLOCK_ALL, data);
	if (!ret)
		ret = event_ret;

	return ret;
}

int amdgv_gpumon_alloc_vf(amdgv_dev_t dev, enum amdgv_set_vf_opt_type opt_type,
			  struct amdgv_vf_option *opt)
{
	/* Ignore opt_type */
	return amdgv_allocate_vf(dev, opt);
}

int amdgv_gpumon_set_vf_num(amdgv_dev_t dev, uint8_t number_vfs)
{
	int ret;
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	data.gpumon_data.type = GPUMON_SET_VF_NUM;
	data.gpumon_data.val = number_vfs;
	data.gpumon_data.result = &event_ret;

	ret = amdgv_sched_queue_event_and_wait_ex(
		adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_GPUMON, AMDGV_SCHED_BLOCK_ALL, data);
	if (!ret)
		ret = event_ret;

	return ret;
}

int amdgv_gpumon_free_vf(amdgv_dev_t dev, uint32_t idx_vf)
{
	return amdgv_free_vf(dev, idx_vf);
}

int amdgv_gpumon_set_vf(amdgv_dev_t dev, enum amdgv_set_vf_opt_type opt_type,
			struct amdgv_vf_option *opt)
{
	return amdgv_set_vf(dev, opt_type, opt);
}

/* currently only consider the manual scheduler gfx active time
 * the auto scheduler need be expanded in the future
 * */
void amdgv_gpumon_update_load_start_time(struct amdgv_adapter *adapt, uint32_t idx_vf,
					 struct amdgv_sched_world_switch *world_switch,
					 bool is_init)
{
	struct amdgv_time_log *time_log;
	uint32_t hw_sched_id = 0;

	/* use switch statement in case we add more block in future */
	switch (world_switch->sched_block) {
	case AMDGV_SCHED_BLOCK_GFX:
		for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
			time_log = &adapt->array_vf[idx_vf].time_log[hw_sched_id];
			time_log->last_load_start = oss_get_time_stamp();

			if (is_init)
				time_log->historical_active_time_data =
					time_log->cumulative_active_time;
		}
		break;
	default:
		break;
	}
	return;
}

void amdgv_gpumon_update_save_end_time(struct amdgv_adapter *adapt, uint32_t idx_vf,
				       struct amdgv_sched_world_switch *world_switch)
{
	struct amdgv_time_log *time_log;
	uint32_t hw_sched_id = 0;

	/* use switch statement in case we add more block in future */
	switch (world_switch->sched_block) {
	case AMDGV_SCHED_BLOCK_GFX:
		for_each_id(hw_sched_id, world_switch->hw_sched_mask) {
			time_log = &adapt->array_vf[idx_vf].time_log[hw_sched_id];
			time_log->last_save_end = oss_get_time_stamp();
			time_log->cumulative_active_time +=
				time_log->last_save_end - time_log->last_load_start;
		}
		break;
	default:
		break;
	}
	return;
}

int amdgv_gpumon_get_product_info(amdgv_dev_t dev, struct amdgv_product_info *info)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_FAILURE;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	data.gpumon_data.type = GPUMON_GET_FRU_PRODUCT_INFO;
	data.gpumon_data.result = &event_ret;

	/* cache product information if not visit */
	if (adapt->product_info.visit) {
		ret = 0;
	} else if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->get_fru_product_info && info) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
		if (!event_ret)
			AMDGV_WARN("Failed to get fru product info\n");
	} else {
		ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	}

	/* simply copy the entire struct for now, assume it is static */
	oss_memcpy(info, &adapt->product_info, sizeof(struct amdgv_product_info));

	return ret;
}

int amdgv_gpumon_control_fru_sigout(amdgv_dev_t dev, const uint8_t *passphrase)
{
	struct amdgv_adapter *adapt;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->control_fru_sigout)
		ret = adapt->pp.pp_funcs->control_fru_sigout(adapt, passphrase);

	return ret;
}

int amdgv_gpumon_get_fw_err_records(amdgv_dev_t dev, uint8_t *num_records,
				    struct amdgv_psp_mb_err_record *err_records)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int event_ret = 0;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!amdgv_psp_fw_attestation_support(adapt)) {
		AMDGV_WARN("FW attestation is not supported\n");
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	}

	data.gpumon_data.type = GPUMON_GET_FW_ERR_RECORDS;
	data.gpumon_data.fw_err_records.num_err_records = num_records;
	data.gpumon_data.fw_err_records.err_records = err_records;
	data.gpumon_data.result = &event_ret;

	ret = amdgv_sched_queue_event_and_wait_ex(
		adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_GPUMON, AMDGV_SCHED_BLOCK_ALL, data);
	if (!ret)
		ret = event_ret;

	return ret;
}

int amdgv_gpumon_get_vf_fw_info(amdgv_dev_t dev, uint32_t idx_vf, uint8_t *num_fw,
				struct amdgv_firmware_info *fw_info)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int event_ret = 0;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!amdgv_psp_fw_attestation_support(adapt)) {
		AMDGV_WARN("FW attestation is not supported\n");
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	}

	data.gpumon_data.type = GPUMON_GET_VF_FW_INFO;
	data.gpumon_data.fw_info.fws = fw_info;
	data.gpumon_data.fw_info.num_fw = num_fw;
	data.gpumon_data.fw_info.idx_vf = idx_vf;
	data.gpumon_data.result = &event_ret;

	ret = amdgv_sched_queue_event_and_wait_ex(adapt, idx_vf, AMDGV_EVENT_SCHED_GPUMON,
						  AMDGV_SCHED_BLOCK_ALL, data);
	if (!ret)
		ret = event_ret;

	return ret;
}

static int amdgv_get_fw_err_records(struct amdgv_adapter *adapt, uint8_t *num_records,
				    struct amdgv_psp_mb_err_record *err_records)
{
	int i, j;

	j = 0;
	for (i = 0; i < AMDGV_MAX_PSP_MB_ERROR_RECORD; i++) {
		if (adapt->error_record[i].valid) {
			oss_memcpy(&err_records[j], &adapt->error_record[i],
				   sizeof(struct amdgv_psp_mb_err_record));
			j++;
		}
	}
	*num_records = j;

	return 0;
}

static int amdgv_get_vf_fw_info(struct amdgv_adapter *adapt, uint32_t idx_vf, uint8_t *num_fw,
				struct amdgv_firmware_info *fw_info)
{
	struct amdgv_vf_device *vf = NULL;

	vf = &adapt->array_vf[idx_vf];

	oss_memcpy(fw_info, &vf->fw_info,
		   (AMDGV_FIRMWARE_ID__MAX * sizeof(struct amdgv_firmware_info)));

	*num_fw = adapt->psp.fw_num - 1;

	return 0;
}


int amdgv_gpumon_get_vram_info(amdgv_dev_t dev, struct amdgv_gpumon_vram_info *vram_info)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!vram_info)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = vram_info;
	data.gpumon_data.type = GPUMON_GET_VRAM_INFO;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->get_vram_info && vram_info) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_is_clk_locked(amdgv_dev_t dev, enum AMDGV_PP_CLK_DOMAIN clk_domain, uint8_t *clk_locked)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!clk_locked)
		return AMDGV_FAILURE;

	data.gpumon_data.clk_locked.locked = clk_locked;
	data.gpumon_data.clk_locked.clk_domain = clk_domain;
	data.gpumon_data.type = GPUMON_IS_CLK_LOCKED;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs && adapt->gpumon.funcs->is_clk_locked && clk_locked) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

static inline const char *amdgv_get_memory_partition_mode_desc(
	enum amdgv_memory_partition_mode memory_partition_mode)
{
	switch (memory_partition_mode) {
	case AMDGV_MEMORY_PARTITION_MODE_NPS1:
		return "NPS1";
	case AMDGV_MEMORY_PARTITION_MODE_NPS2:
		return "NPS2";
	case AMDGV_MEMORY_PARTITION_MODE_NPS4:
		return "NPS4";
	case AMDGV_MEMORY_PARTITION_MODE_NPS8:
		return "NPS8";
	default:
		return "UNKNOWN";
	}
}

static bool amdgv_gpumon_is_change_partition_mode(struct amdgv_adapter *adapt)
{
	uint32_t vf_assignment = 0;

	/* check OS VF assignment */
	vf_assignment = oss_get_assigned_vf_count(adapt->dev, false);
	if (vf_assignment == OSS_FUNCTION_NOT_IMPLEMENTED)
		vf_assignment = 0;

	/* check libgv VF assignment, simply OR is enough */
	vf_assignment |= amdgv_get_vf_candidate(adapt);

	/* return true if no assignment */
	return (vf_assignment == 0);
}

int amdgv_gpumon_set_memory_partition_mode(
	amdgv_dev_t dev,
	enum amdgv_memory_partition_mode memory_partition_mode)
{
	struct amdgv_adapter *adapt;
	struct amdgv_adapter *next_adapt;
	struct amdgv_hive_info *hive;
	struct amdgv_gpumon_memory_partition_info curr_memory_partition_info;

	bool is_change_mode = true;
	bool is_need_reset = false;
	int tmp = 0;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (memory_partition_mode == AMDGV_MEMORY_PARTITION_MODE_UNKNOWN) {
		AMDGV_ERROR("invalid memory partition mode\n");
		return AMDGV_ERROR_GPUMON_INVALID_MODE;
	}

	if (amdgv_gpumon_get_memory_partition_mode(
		    dev, &curr_memory_partition_info)) {
		AMDGV_ERROR("failed to get current memory partition mode\n");
		return AMDGV_FAILURE;
	}

	/* check if previous set memory partition mode in progress */
	if (adapt->mcp.mem_mode_switch_requested) {
		AMDGV_INFO("memory mode switch in progress. current memory partition mode is already set to %s\n",
			amdgv_get_memory_partition_mode_desc(adapt->mcp.memory_partition_mode));
		return AMDGV_ERROR_GPUMON_SET_ALREADY;
	}

	/* check if requested memory partition mode is same as current */
	if (curr_memory_partition_info.memory_partition_mode == memory_partition_mode) {
		AMDGV_INFO("current memory partition mode is already set to %s\n",
			   amdgv_get_memory_partition_mode_desc(
				   memory_partition_mode));
		return AMDGV_ERROR_GPUMON_SET_ALREADY;
	}

	hive = amdgv_get_xgmi_hive(adapt);
	if (hive) {
		oss_mutex_lock(adapt->gpumon_hive_lock);
		amdgv_list_for_each_entry(next_adapt, &hive->adapt_list, struct amdgv_adapter, xgmi.head) {
			is_change_mode &= amdgv_gpumon_is_change_partition_mode(next_adapt);
		}
	} else {
		is_change_mode = amdgv_gpumon_is_change_partition_mode(adapt);
	}

	if (!is_change_mode) {
		AMDGV_ERROR("set memory partition mode not allowed when active VM/VFs running!\n");
		ret = AMDGV_ERROR_GPUMON_VF_BUSY;
		goto unlock;
	}

	if (hive) {
		amdgv_list_for_each_entry(next_adapt, &hive->adapt_list, struct amdgv_adapter, xgmi.head) {
			tmp = amdgv_set_memory_partition_mode(next_adapt, memory_partition_mode);
			if (tmp == 0)
				is_need_reset = true;
			ret |= tmp;
		}
	} else {
		ret = amdgv_set_memory_partition_mode(adapt, memory_partition_mode);
		if (ret == 0)
			is_need_reset = true;
	}

unlock:
	if (hive)
		oss_mutex_unlock(adapt->gpumon_hive_lock);

	if (is_need_reset)
		amdgv_sched_queue_event(adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_FORCE_RESET_GPU, 0);

	return ret;
}

int amdgv_gpumon_set_spatial_partition_num(amdgv_dev_t dev,
					   uint32_t spatial_partition_num)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	data.gpumon_data.sp.spatial_partition_num = spatial_partition_num;
	data.gpumon_data.type = GPUMON_SET_SPATIAL_PARTITION_NUM;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
	    adapt->gpumon.funcs->set_spatial_partition_num) {
		ret = amdgv_sched_queue_event_and_wait_ex(
			adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_GPUMON,
			AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_set_accelerator_partition_profile(
	amdgv_dev_t dev, uint32_t accelerator_partition_profile_index)
{
	struct amdgv_adapter *adapt;
	struct amdgv_adapter *next_adapt;
	struct amdgv_hive_info *hive;
	struct amdgv_gpumon_acccelerator_partition_profile
		accelerator_partition_profile;

	bool is_change_mode = true;
	int ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	/* check if requested accelerator_partition_profile is same as current */
	if (amdgv_gpumon_get_accelerator_partition_profile(
		    dev, &accelerator_partition_profile)) {
		AMDGV_ERROR("failed to get accelerator_partition_profile\n");
		return AMDGV_FAILURE;
	}
	if (accelerator_partition_profile.profile_index ==
	    accelerator_partition_profile_index) {
		AMDGV_INFO(
			"current accelerator_partition_profile is already set to profile_index=%u\n",
			accelerator_partition_profile_index);
		return AMDGV_ERROR_GPUMON_SET_ALREADY;
	}

	hive = amdgv_get_xgmi_hive(adapt);
	if (hive) {
		oss_mutex_lock(adapt->gpumon_hive_lock);
		amdgv_list_for_each_entry(next_adapt, &hive->adapt_list, struct amdgv_adapter, xgmi.head) {
			is_change_mode &= amdgv_gpumon_is_change_partition_mode(next_adapt);
		}
	} else {
		is_change_mode = amdgv_gpumon_is_change_partition_mode(adapt);
	}

	if (!is_change_mode) {
		AMDGV_ERROR("set accelerator partition mode not allowed when active VM/VFs running!\n");
		ret = AMDGV_ERROR_GPUMON_VF_BUSY;
		goto unlock;
	}

	if (hive) {
		amdgv_list_for_each_entry(next_adapt, &hive->adapt_list, struct amdgv_adapter, xgmi.head) {
			ret |= amdgv_set_accelerator_partition_profile(next_adapt, accelerator_partition_profile_index);
		}
	} else {
		ret = amdgv_set_accelerator_partition_profile(adapt, accelerator_partition_profile_index);
	}

unlock:
	if (hive)
		oss_mutex_unlock(adapt->gpumon_hive_lock);

	return ret;
}

int amdgv_gpumon_reset_partition_mode(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	struct amdgv_gpumon_accelerator_partition_profile_config *accelerator_partition_profile_config;
	uint32_t i;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	/* reset memory partition mode to NPS1 */
	ret = amdgv_gpumon_set_memory_partition_mode(dev,
			AMDGV_MEMORY_PARTITION_MODE_NPS1);
	if (ret) {
		AMDGV_ERROR("failed to reset memory partition mode\n");
		return AMDGV_FAILURE;
	}

	/* reset accelerator partition mode to 1VF:SPX 2VF:DPX 4VF:QPX 8VF:CPX */
	accelerator_partition_profile_config = oss_zalloc(
		sizeof(struct amdgv_gpumon_accelerator_partition_profile_config));
	if (accelerator_partition_profile_config == NULL) {
		AMDGV_ERROR("failed to allocate memory for  accelerator partition profile\n");
		return AMDGV_FAILURE;
	}
	ret = amdgv_gpumon_get_accelerator_partition_profile_config(dev,
			accelerator_partition_profile_config);
	if (ret) {
		AMDGV_ERROR("failed to get accelerator partition config\n");
		goto out;
	}
	for (i = 0; i < accelerator_partition_profile_config->number_of_profiles; i++) {
		if (accelerator_partition_profile_config->profiles[i].num_partitions == adapt->num_vf) {
			ret = amdgv_gpumon_set_accelerator_partition_profile(dev, i);
			if (ret) {
				AMDGV_ERROR("failed to reset accelerator partition mode\n");
				goto out;
			}
			break;
		}
	}

out:
	oss_free(accelerator_partition_profile_config);
	return ret;
}

int amdgv_gpumon_reset_spatial_partition_num(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	data.gpumon_data.type = GPUMON_RESET_SPATIAL_PARTITION_NUM;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
	    adapt->gpumon.funcs->reset_spatial_partition_num) {
		ret = amdgv_sched_queue_event_and_wait_ex(
			adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_GPUMON,
			AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_accelerator_partition_profile_config(
	amdgv_dev_t dev,
	struct amdgv_gpumon_accelerator_partition_profile_config
		*profile_configs)
{
	struct amdgv_adapter *adapt;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!profile_configs)
		return AMDGV_FAILURE;

	if (adapt->gpumon.funcs &&
	    adapt->gpumon.funcs->get_accelerator_partition_profile_config &&
	    profile_configs) {
		ret = adapt->gpumon.funcs
			      ->get_accelerator_partition_profile_config(
				      adapt, profile_configs);
	}

	return ret;
}

int amdgv_gpumon_get_memory_partition_config(
	amdgv_dev_t dev,
	union amdgv_gpumon_memory_partition_config *memory_partition_config)
{
	struct amdgv_adapter *adapt;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!memory_partition_config)
		return AMDGV_FAILURE;

	if (adapt->gpumon.funcs &&
	    adapt->gpumon.funcs->get_memory_partition_config &&
	    memory_partition_config) {
		ret = adapt->gpumon.funcs->get_memory_partition_config(
			adapt, memory_partition_config);
	}

	return ret;
}

int amdgv_gpumon_get_spatial_partition_caps(
	amdgv_dev_t dev,
	struct amdgv_gpumon_spatial_partition_caps *spatial_partition_caps)
{
	struct amdgv_adapter *adapt;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!spatial_partition_caps)
		return AMDGV_FAILURE;

	if (adapt->gpumon.funcs &&
	    adapt->gpumon.funcs->get_spatial_partition_caps &&
	    spatial_partition_caps) {
		ret = adapt->gpumon.funcs->get_spatial_partition_caps(
			adapt, spatial_partition_caps);
	}

	return ret;
}

int amdgv_gpumon_get_accelerator_partition_profile(
	amdgv_dev_t dev, struct amdgv_gpumon_acccelerator_partition_profile
				 *accelerator_partition_profile)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!accelerator_partition_profile)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = accelerator_partition_profile;
	data.gpumon_data.type = GPUMON_GET_ACCELERATOR_PARTITION_PROFILE;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
	    adapt->gpumon.funcs->get_accelerator_partition_profile &&
	    accelerator_partition_profile) {
		ret = amdgv_sched_queue_event_and_wait_ex(
			adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_GPUMON,
			AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_memory_partition_mode(
	amdgv_dev_t dev,
	struct amdgv_gpumon_memory_partition_info *memory_partition_info)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!memory_partition_info)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = memory_partition_info;
	data.gpumon_data.type = GPUMON_GET_MEMORY_PARTITION_MODE;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
	    adapt->gpumon.funcs->get_memory_partition_mode &&
	    memory_partition_info) {
		ret = amdgv_sched_queue_event_and_wait_ex(
			adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_GPUMON,
			AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_spatial_partition_num(amdgv_dev_t dev,
					   uint32_t *spatial_partition_num)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!spatial_partition_num)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = spatial_partition_num;
	data.gpumon_data.type = GPUMON_GET_SPATIAL_PARTITION_NUM;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
	    adapt->gpumon.funcs->get_spatial_partition_num &&
	    spatial_partition_num) {
		ret = amdgv_sched_queue_event_and_wait_ex(
			adapt, AMDGV_PF_IDX, AMDGV_EVENT_SCHED_GPUMON,
			AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_pcie_replay_count(amdgv_dev_t dev,
		int *pcie_replay_count)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!pcie_replay_count)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = pcie_replay_count;
	data.gpumon_data.type = GPUMON_GET_PCIE_REPLAY_COUNT;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
		adapt->gpumon.funcs->get_pcie_replay_count &&
		pcie_replay_count) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_card_form_factor(amdgv_dev_t dev,
			enum amdgv_gpumon_card_form_factor *card_form_factor)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!card_form_factor)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = card_form_factor;
	data.gpumon_data.type = GPUMON_GET_CARD_FORM_FACTOR;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
		adapt->gpumon.funcs->get_card_form_factor &&
		card_form_factor) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_max_configurable_power_limit(amdgv_dev_t dev,
			int *power_limit)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!power_limit)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = power_limit;
	data.gpumon_data.type = GPUMON_GET_MAX_CONFIG_POWER_LIMIT;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
		adapt->gpumon.funcs->get_max_configurable_power_limit &&
		power_limit) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_default_power_limit(amdgv_dev_t dev,
			int *power_limit)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!power_limit)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = power_limit;
	data.gpumon_data.type = GPUMON_GET_DEFAULT_POWER_LIMIT;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
		adapt->gpumon.funcs->get_default_power_limit &&
		power_limit) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_min_power_limit(amdgv_dev_t dev,
			int *min_power_limit)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!min_power_limit)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = min_power_limit;
	data.gpumon_data.type = GPUMON_GET_MIN_POWER_LIMIT;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
		adapt->gpumon.funcs->get_min_power_limit &&
		min_power_limit) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_metrics_ext(amdgv_dev_t dev,
			struct amdgv_gpumon_metrics_ext *metrics_ext)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!metrics_ext)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = metrics_ext;
	data.gpumon_data.type = GPUMON_GET_METRICS_EXT;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
		adapt->gpumon.funcs->get_metrics_ext &&
		metrics_ext) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_num_metrics_ext_entries(amdgv_dev_t dev,
			uint32_t *entries)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!entries)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = entries;
	data.gpumon_data.type = GPUMON_GET_NUM_METRICS_EXT_ENTRIES;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
		adapt->gpumon.funcs->get_num_metrics_ext_entries &&
		entries) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}
	return ret;
}

int amdgv_gpumon_is_power_management_enabled(amdgv_dev_t dev,
			bool *pm_enabled)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!pm_enabled)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = pm_enabled;
	data.gpumon_data.type = GPUMON_IS_POWER_MANAGEMENT_ENABLED;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
		adapt->gpumon.funcs->is_power_management_enabled &&
		pm_enabled) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_link_metrics(amdgv_dev_t dev,
		struct amdgv_gpumon_link_metrics *link_metrics)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!link_metrics)
		return AMDGV_FAILURE;

	oss_memset(link_metrics, 0, sizeof(struct amdgv_gpumon_link_metrics));

	oss_mutex_lock(adapt->gpumon_hive_lock);

	data.gpumon_data.ptr = link_metrics;
	data.gpumon_data.type = GPUMON_GET_LINK_METRICS;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
		adapt->gpumon.funcs->get_link_metrics &&
		link_metrics) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	oss_mutex_unlock(adapt->gpumon_hive_lock);

	return ret;
}

int amdgv_gpumon_get_link_topology(amdgv_dev_t src_dev,
			amdgv_dev_t dest_dev, struct amdgv_gpumon_link_topology_info *topology_info)
{
	struct amdgv_adapter *src_adapt;
	struct amdgv_adapter *dest_adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(src_adapt, src_dev);
	SET_ADAPT_AND_CHECK_STATUS(dest_adapt, dest_dev);

	if (!topology_info)
		return AMDGV_FAILURE;

	oss_mutex_lock(src_adapt->gpumon_hive_lock);

	data.gpumon_data.xgmi_node_topology.dest_adapt = dest_adapt;
	data.gpumon_data.xgmi_node_topology.topology_info = topology_info;
	data.gpumon_data.type = GPUMON_GET_LINK_TOPOLOGY;
	data.gpumon_data.result = &event_ret;

	if (src_adapt->gpumon.funcs &&
		src_adapt->gpumon.funcs->get_link_topology &&
		topology_info) {
		ret = amdgv_sched_queue_event_and_wait_ex(src_adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	oss_mutex_unlock(src_adapt->gpumon_hive_lock);

	return ret;
}

int amdgv_gpumon_get_xgmi_fb_sharing_caps(amdgv_dev_t dev,
			union amdgv_gpumon_xgmi_fb_sharing_caps *caps)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!caps)
		return AMDGV_FAILURE;

	oss_mutex_lock(adapt->gpumon_hive_lock);

	data.gpumon_data.ptr = caps;
	data.gpumon_data.type = GPUMON_GET_XGMI_FB_SHARING_CAPS;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
		adapt->gpumon.funcs->get_xgmi_fb_sharing_caps &&
		caps) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	oss_mutex_unlock(adapt->gpumon_hive_lock);

	return ret;
}

int amdgv_gpumon_get_xgmi_fb_sharing_mode_info(amdgv_dev_t src_dev,
			amdgv_dev_t dest_dev, enum amdgv_gpumon_xgmi_fb_sharing_mode mode,
			uint8_t *is_sharing_enabled)
{
	struct amdgv_adapter *src_adapt;
	struct amdgv_adapter *dest_adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(src_adapt, src_dev);
	SET_ADAPT_AND_CHECK_STATUS(dest_adapt, dest_dev);

	if (!is_sharing_enabled)
		return AMDGV_FAILURE;

	oss_mutex_lock(src_adapt->gpumon_hive_lock);

	data.gpumon_data.xgmi_fb_sharing_mode_info.dest_adapt = dest_adapt;
	data.gpumon_data.xgmi_fb_sharing_mode_info.mode = mode;
	data.gpumon_data.xgmi_fb_sharing_mode_info.is_sharing_enabled = is_sharing_enabled;
	data.gpumon_data.type = GPUMON_GET_XGMI_FB_SHARING_MODE_INFO;
	data.gpumon_data.result = &event_ret;

	if (src_adapt->gpumon.funcs &&
		src_adapt->gpumon.funcs->get_link_topology &&
		is_sharing_enabled) {
		ret = amdgv_sched_queue_event_and_wait_ex(src_adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	oss_mutex_unlock(src_adapt->gpumon_hive_lock);

	return ret;
}

int amdgv_gpumon_set_xgmi_fb_sharing_mode(amdgv_dev_t dev,
			enum amdgv_gpumon_xgmi_fb_sharing_mode mode)
{
	struct amdgv_adapter *adapt;
	struct amdgv_adapter *next_adapt;
	struct amdgv_hive_info *hive;
	union amdgv_sched_event_data data;
	int ret = AMDGV_FAILURE;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (amdgv_xgmi_has_bad_gpu_in_hive(adapt))
		return ret;

	hive = amdgv_get_xgmi_hive(adapt);
	if (!hive) {
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	}

	oss_mutex_lock(adapt->gpumon_hive_lock);

	amdgv_list_for_each_entry(next_adapt, &hive->adapt_list,
				struct amdgv_adapter, xgmi.head) {
		if (amdgv_sched_vf_assigned_to_vm(next_adapt)) {
			ret = AMDGV_ERROR_GPUMON_VF_BUSY;
			goto out;
		}
	}

	amdgv_list_for_each_entry(next_adapt, &hive->adapt_list,
				  struct amdgv_adapter, xgmi.head) {
		data.gpumon_data.xgmi_set_fb_sharing_mode.mode = mode;
		data.gpumon_data.type = GPUMON_SET_XGMI_FB_SHARING_MODE;
		data.gpumon_data.result = &event_ret;
		if (!(next_adapt->gpumon.funcs &&
			next_adapt->gpumon.funcs->set_xgmi_fb_sharing_mode)) {
			ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
			goto out;
		}
		ret = amdgv_sched_queue_event_and_wait_ex(next_adapt, AMDGV_PF_IDX,
						AMDGV_EVENT_SCHED_GPUMON,
						AMDGV_SCHED_BLOCK_ALL, data);
		if (ret || event_ret) {
			ret = event_ret;
			goto out;
		}
	}

out:
	oss_mutex_unlock(adapt->gpumon_hive_lock);

	return ret;
}

int amdgv_gpumon_get_pm_policy(amdgv_dev_t dev,
			enum amdgv_pp_pm_policy p_type,
			struct amdgv_gpumon_smu_dpm_policy *policy)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	switch (p_type) {
	case AMDGV_PP_PM_POLICY_SOC_PSTATE:
		data.gpumon_data.type = GPUMON_GET_DPM_POLICY_LEVEL;
		break;
	case AMDGV_PP_PM_POLICY_XGMI_PLPD:
		data.gpumon_data.type = GPUMON_GET_XGMI_PLPD_POLICY_LEVEL;
		break;
	default:
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	}

	data.gpumon_data.pm_policy.dpm_policy = policy;
	data.gpumon_data.result = &event_ret;


	if (adapt->gpumon.funcs &&
		adapt->gpumon.funcs->get_pm_policy) {
			ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		}
	if (!ret)
		ret = event_ret;

	return ret;
}

int amdgv_gpumon_set_pm_policy_level(amdgv_dev_t dev,
			enum amdgv_pp_pm_policy p_type, int level)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	data.gpumon_data.pm_info.p_type = p_type;
	data.gpumon_data.pm_info.level = level;
	data.gpumon_data.type = GPUMON_SET_PM_POLICY_LEVEL;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
		adapt->gpumon.funcs->set_pm_policy_level) {
			ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		}

	if (!ret)
		ret = event_ret;

	return ret;
}

#define GET_XGMI_PHY_NODE_MASK(adapt) (1 << adapt->xgmi.phy_node_id)
#define XGMI_NODE_PER_NUMA 4
/* 0x0f */
#define LOW_NUMA_BIT_MASK ((1 << XGMI_NODE_PER_NUMA) - 1)
/* 0xf0 */
#define HIGH_NUMA_BIT_MASK (LOW_NUMA_BIT_MASK << XGMI_NODE_PER_NUMA)
#define LOW_NUMA_HAS_NODE(share_mask) (share_mask & LOW_NUMA_BIT_MASK)
#define HIGH_NUMA_HAS_NODE(share_mask) (share_mask & HIGH_NUMA_BIT_MASK)
/* high/low numa_has_node both true or both false, (not xor)*/
#define SHARING_MASK_IS_CROSS_NUMA_OR_EMPTY(share_mask)\
			 ((HIGH_NUMA_HAS_NODE(share_mask) && LOW_NUMA_HAS_NODE(share_mask)) ||\
			 !(HIGH_NUMA_HAS_NODE(share_mask) || LOW_NUMA_HAS_NODE(share_mask)))
/** amdgv_gpumon_set_xgmi_fb_custom_sharing_mode - set custom fb sharing mode
 *
 * @dev_list_size: number of dev to set shared with each other
 * @dev_list: list of dev
 *
 * This function attempt to set specified dev in dev_list as fb shared (sharing group)
 *
 * adapt->xgmi.custom_mode_sharing_mask is used to store if a adapt is shared with
 * other adapt
 *
 * Several checks are performed if the specified setting is allowed:
 * - specified device are in the same numa node
 * - specified device list size is in 1/2/4
 * - affected device has no active vf
 *
 * Affected device are derived by checking if a device is in a sharing group that has
 * overlap with input sharing groups
 *
 * If all above check passed, a AMDGV_GPUMON_XGMI_FB_SHARING_MODE_CUSTOM event is sent
 * to each affected device to apply topology
 */
int amdgv_gpumon_set_xgmi_fb_custom_sharing_mode(uint32_t dev_list_size,
			amdgv_dev_t *dev_list)
{
	struct amdgv_adapter *adapt;
	struct amdgv_adapter *next_adapt;
	struct amdgv_hive_info *hive;
	union amdgv_sched_event_data data;
	int ret = AMDGV_FAILURE;
	int event_ret = 0;
	amdgv_dev_t *loop_dev;
	uint32_t share_enable_mask = 0;
	uint32_t target_mask_list[AMDGV_XGMI_MAX_CONNECTED_NODES/AMDGV_MAX_XGMI_HIVE + 1];
	enum amdgv_gpumon_xgmi_fb_sharing_mode current_mode;
	int hive_adapt_count = 0;

	/* set true when entering custom mode from other modes */
	bool non_custom_to_custom_mode = false;
	int i = 0;

	/* allow  1 2 4 sharing only */
	if (!(dev_list_size == 1 || dev_list_size == 2 || dev_list_size == 4))
		return AMDGV_ERROR_GPUMON_INVALID_OPTION;
	/* build sharing mask */
	for (i = 0; i < dev_list_size; i++) {
		loop_dev = dev_list[i];
		SET_ADAPT_AND_CHECK_STATUS(next_adapt, loop_dev);
		share_enable_mask |= GET_XGMI_PHY_NODE_MASK(next_adapt);
	}
	/* avoid cross numa share*/
	if (SHARING_MASK_IS_CROSS_NUMA_OR_EMPTY(share_enable_mask)) {
		return AMDGV_ERROR_GPUMON_INVALID_OPTION;
	}

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev_list[0]);
	current_mode = adapt->xgmi.fb_sharing_mode;

	/* entering from other modes to custom mode */
	if (adapt->xgmi.fb_sharing_mode != amdgv_gpumon_xgmi_mode_map(AMDGV_GPUMON_XGMI_FB_SHARING_MODE_CUSTOM)) {
		non_custom_to_custom_mode = true;
	}

	hive = amdgv_get_xgmi_hive(adapt);
	if (!hive) {
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	}

	oss_mutex_lock(adapt->gpumon_hive_lock);

	/* abort when any affected node has active vf */
	amdgv_list_for_each_entry(next_adapt, &hive->adapt_list,
				struct amdgv_adapter, xgmi.head) {
		if (non_custom_to_custom_mode) {
			/**
			 * in custom mode initialization
			 * target mask has 3 cases
			 * - share enable mask if specified node is inside share enalble mask
			 * - PHY_NODE_MASK if node is affected but outside share enalble mask
			 * - default to current mode equivalent sharing mask
			 */
			if (next_adapt->xgmi.get_fb_sharing_mode_mask) {
				/* save equivelant sharing mask to target_mask_list
				 * target mask will later be updated to apply share_enable_mask
				 */
				target_mask_list[hive_adapt_count] =
					 next_adapt->xgmi.get_fb_sharing_mode_mask(next_adapt, current_mode);
			} else {
					ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
					goto out;
			}

			if (target_mask_list[hive_adapt_count] & share_enable_mask) {
				/* loop affected nodes*/
				if (amdgv_sched_vf_assigned_to_vm(next_adapt)) {
					ret = AMDGV_ERROR_GPUMON_VF_BUSY;
					goto out;
				}
				/* update target mask */
				target_mask_list[hive_adapt_count] =
					 (share_enable_mask & GET_XGMI_PHY_NODE_MASK(next_adapt)) ?
					  share_enable_mask : GET_XGMI_PHY_NODE_MASK(next_adapt);
			}

			hive_adapt_count++;
		} else if (next_adapt->xgmi.custom_mode_sharing_mask & share_enable_mask) {
			/* loop affected nodes or loop all node when initializing custom mode */
			if (amdgv_sched_vf_assigned_to_vm(next_adapt)) {
				ret = AMDGV_ERROR_GPUMON_VF_BUSY;
				goto out;
			}
		}
	}

	i = 0;
	/* set sharing mask according to sent dev list, remove all touched dev and set specified dev shared */
	amdgv_list_for_each_entry(next_adapt, &hive->adapt_list,
				  struct amdgv_adapter, xgmi.head) {
		if ((next_adapt->xgmi.custom_mode_sharing_mask & share_enable_mask) || non_custom_to_custom_mode) {
		/* loop affected nodes or loop all node when initializing custom mode */
			data.gpumon_data.xgmi_set_fb_sharing_mode.mode = AMDGV_GPUMON_XGMI_FB_SHARING_MODE_CUSTOM;
			/* sharing_mask to set has 3 cases
			 *
			 * - use target mask list when initializing custom mode
			 *  	when initializing custom mode, target mask list has 3 cases, see above
			 * - for affected node, if it is inside share enable mask, use share enable mask
			 * - default to PHY_NODE_MASK which only contain the node itself
			 */
			data.gpumon_data.xgmi_set_fb_sharing_mode.sharing_mask = GET_XGMI_PHY_NODE_MASK(next_adapt);
			if (non_custom_to_custom_mode) {
				/* use target_mask_list when initializing custom mode */
				data.gpumon_data.xgmi_set_fb_sharing_mode.sharing_mask = target_mask_list[i++];
			} else if (GET_XGMI_PHY_NODE_MASK(next_adapt) & share_enable_mask) {
				/* set share_enable_mask specified node's sharing mask */
				data.gpumon_data.xgmi_set_fb_sharing_mode.sharing_mask = share_enable_mask;
			}
			data.gpumon_data.type = GPUMON_SET_XGMI_FB_SHARING_MODE_EX;
			data.gpumon_data.result = &event_ret;

			if (!(next_adapt->gpumon.funcs &&
				next_adapt->gpumon.funcs->set_xgmi_fb_sharing_mode_ex)) {
				ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
				goto out;
			}
			ret = amdgv_sched_queue_event_and_wait_ex(next_adapt, AMDGV_PF_IDX,
							AMDGV_EVENT_SCHED_GPUMON,
							AMDGV_SCHED_BLOCK_ALL, data);
			if (ret || event_ret) {
				ret = event_ret;
				goto out;
			}
		}
	}

out:
	oss_mutex_unlock(adapt->gpumon_hive_lock);

	return ret;
}


int amdgv_gpumon_get_shutdown_temperature(amdgv_dev_t dev,
	int *shutdown_temp)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!shutdown_temp)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = shutdown_temp;
	data.gpumon_data.type = GPUMON_GET_SHUTDOWN_TEMP;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
		adapt->gpumon.funcs->get_shutdown_temperature &&
		shutdown_temp) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_gpu_cache_info(amdgv_dev_t dev,
			struct amdgv_gpumon_gpu_cache_info *gpu_cache_info)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!gpu_cache_info)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = gpu_cache_info;
	data.gpumon_data.type = GPUMON_GET_GPU_CACHE_INFO;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
		adapt->gpumon.funcs->get_gpu_cache_info &&
		gpu_cache_info) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

int amdgv_gpumon_get_gpu_max_pcie_link_generation(amdgv_dev_t dev,
			int *pcie_max_link_gen)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	if (!pcie_max_link_gen)
		return AMDGV_FAILURE;

	data.gpumon_data.ptr = pcie_max_link_gen;
	data.gpumon_data.type = GPUMON_GET_MAX_PCIE_LINK_GENERATION;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
		adapt->gpumon.funcs->get_max_pcie_link_generation &&
		pcie_max_link_gen) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}


int amdgv_gpumon_get_ras_safe_fb_addr_ranges(amdgv_dev_t dev,
	struct amdgv_gpumon_ras_safe_fb_address_ranges *ranges)
{
	struct amdgv_adapter *adapt;
	int ret = 0;
	uint32_t i;

	uint64_t *offset_array = NULL;
	uint64_t *size_array   = NULL;

	if (!ranges)
		return AMDGV_FAILURE;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	offset_array = oss_malloc(AMDGV_GPUMON_RAS_MAX_NUM_SAFE_RANGES * sizeof(uint64_t));
	size_array   = oss_malloc(AMDGV_GPUMON_RAS_MAX_NUM_SAFE_RANGES * sizeof(uint64_t));

	if (offset_array == NULL || size_array == NULL) {
		AMDGV_ERROR("failed to allocate memory for RAS safe ranges!\n");
		ret = AMDGV_FAILURE;
		goto free;
	}

	ranges->num_ranges = amdgv_umc_get_ras_vf_safe_range(adapt,
		(uint64_t *)offset_array, (uint64_t *)size_array, AMDGV_GPUMON_RAS_MAX_NUM_SAFE_RANGES);

	if (ranges->num_ranges > AMDGV_GPUMON_RAS_MAX_NUM_SAFE_RANGES) {
		AMDGV_ERROR("exceed the max of number of RAS safe ranges!\n");
		ret = AMDGV_FAILURE;
		goto free;
	}

	for (i = 0; i < ranges->num_ranges; i++) {
		ranges->range[i].start = offset_array[i];
		ranges->range[i].size  = size_array[i];
	}

free:
	if (offset_array)
		oss_free(offset_array);

	if (size_array)
		oss_free(size_array);

	return ret;
}

int amdgv_gpumon_get_ras_session_id(amdgv_dev_t dev, uint64_t *session_ptr)
{
	struct amdgv_adapter *adapt;
	struct psp_ras_context *ras_context;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	ras_context = &(adapt->psp.ras_context);

	if (ras_context->ras_initialized) {
		*session_ptr = ras_context->ras_session_id;
	} else {
		AMDGV_ERROR("failed to get ras session id!\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

int amdgv_gpumon_ras_get_ta_version(amdgv_dev_t dev, unsigned char *fw_image, uint32_t *ver_ptr)
{
	struct amdgv_adapter *adapt;
	uint32_t ret;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	ret = amdgv_psp_ras_get_ta_version(adapt, (uint8_t *)fw_image, ver_ptr);
	if (ret) {
		AMDGV_ERROR("failed to get psp ta version!\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

int amdgv_gpumon_ras_get_loaded_ta_version(amdgv_dev_t dev, uint32_t *ver_ptr)
{
	struct amdgv_adapter *adapt;
	struct psp_ras_context *ras_context;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	ras_context = &(adapt->psp.ras_context);

	if (ras_context->ras_initialized) {
		*ver_ptr = adapt->psp.fw_info[AMDGV_FIRMWARE_ID__RAS_TA];
	} else {
		AMDGV_ERROR("failed to get loaded ta version!\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

int amdgv_gpumon_reset_all_error_counts(amdgv_dev_t dev)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_FAILURE;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	data.gpumon_data.type = GPUMON_RESET_ALL_ERROR_COUNTS;
	data.gpumon_data.result = &event_ret;

	ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
	if (!ret)
		ret = event_ret;

	return ret;
}

int amdgv_gpumon_get_num_active_vfs(amdgv_dev_t dev, uint32_t *num_vfs)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	*num_vfs = amdgv_sched_active_vf_ex_pf_num(adapt);

	return 0;
}

int amdgv_gpumon_cper_get_count(amdgv_dev_t dev,
				uint64_t rptr, uint64_t *wptr,
				uint64_t *avail_count,
				uint64_t *size)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS_MINIMAL(adapt, dev);

	if (!adapt->cper.enabled)
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;

	if (!wptr || !avail_count || !size)
		return AMDGV_FAILURE;

	data.gpumon_data.cper.get_count.rptr = rptr;
	data.gpumon_data.cper.get_count.wptr = wptr;
	data.gpumon_data.cper.get_count.avail_count = avail_count;
	data.gpumon_data.cper.get_count.size = size;

	data.gpumon_data.type = GPUMON_CPER_GET_COUNT;
	data.gpumon_data.result = &event_ret;

	ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							AMDGV_EVENT_SCHED_GPUMON,
							AMDGV_SCHED_BLOCK_ALL, data);
	if (!ret)
		ret = event_ret;

	return ret;
}

int amdgv_gpumon_cper_get_entries(amdgv_dev_t dev, uint64_t rptr,
				  void *buf, uint64_t buf_size,
				  uint64_t *write_count,
				  uint64_t *overflow_count,
				  uint64_t *left_size)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	SET_ADAPT_AND_CHECK_STATUS_MINIMAL(adapt, dev);

	if (!adapt->cper.enabled)
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;

	if (!buf || !write_count || !overflow_count)
		return AMDGV_FAILURE;

	data.gpumon_data.cper.get_entries.rptr = rptr;
	data.gpumon_data.cper.get_entries.buf = buf;
	data.gpumon_data.cper.get_entries.buf_size = buf_size;
	data.gpumon_data.cper.get_entries.write_count = write_count;
	data.gpumon_data.cper.get_entries.overflow_count = overflow_count;
	data.gpumon_data.cper.get_entries.left_size = left_size;

	data.gpumon_data.type = GPUMON_CPER_GET_ENTRIES;
	data.gpumon_data.result = &event_ret;

	ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							AMDGV_EVENT_SCHED_GPUMON,
							AMDGV_SCHED_BLOCK_ALL, data);
	if (!ret)
		ret = event_ret;

	return ret;
}

int amdgv_gpumon_get_gfx_config(amdgv_dev_t dev,
			struct amdgv_gpumon_gfx_config *config)
{
	struct amdgv_adapter *adapt;
	union amdgv_sched_event_data data;
	int ret = AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	int event_ret = 0;

	if (!config)
		return AMDGV_FAILURE;

	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	data.gpumon_data.ptr = config;
	data.gpumon_data.type = GPUMON_GET_GFX_CONFIG;
	data.gpumon_data.result = &event_ret;

	if (adapt->gpumon.funcs &&
		adapt->gpumon.funcs->get_gfx_config) {
		ret = amdgv_sched_queue_event_and_wait_ex(adapt, AMDGV_PF_IDX,
							  AMDGV_EVENT_SCHED_GPUMON,
							  AMDGV_SCHED_BLOCK_ALL, data);
		if (!ret)
			ret = event_ret;
	}

	return ret;
}

static void amdgv_gpumon_log_event(struct amdgv_adapter *adapt,
				    struct amdgv_sched_event *event)
{
	if (event->data.gpumon_data.type >= GPUMON_FRU_SIGOUT)
		AMDGV_INFO("process GPUMON event %s (type:%d)\n",
			   amdgv_gpumon_event_name(event->data.gpumon_data.type),
			   event->data.gpumon_data.type);
	else
		AMDGV_DEBUG("process GPUMON event %s (type:%d)\n",
			    amdgv_gpumon_event_name(event->data.gpumon_data.type),
			    event->data.gpumon_data.type);
}


int amdgv_gpumon_handle_sched_event(struct amdgv_adapter *adapt,
				    struct amdgv_sched_event *event)
{
	int ret = 0;
	struct amdgv_gpumon_metrics *metrics = NULL;
	struct amdgv_gpumon_temp *temp = NULL;
	int *val = NULL;
	char *str = NULL;
	uint64_t tstamp;
	int size;

	amdgv_gpumon_log_event(adapt, event);

	switch (event->data.gpumon_data.type) {
	case GPUMON_GET_PP_METRICS:
		if (adapt->gpumon.funcs == NULL ||
		    adapt->gpumon.funcs->get_pp_metrics == NULL) {
			ret = AMDGV_FAILURE;
			*event->data.gpumon_data.result = ret;
			break;
		}

		metrics = event->data.gpumon_data.ptr;

		if (adapt->pp.metrics_cache.metrics == NULL) {
			ret = adapt->gpumon.funcs->get_pp_metrics(adapt, metrics);
			*event->data.gpumon_data.result = ret;
			break;
		}

		tstamp = oss_get_time_stamp();
		size = sizeof(struct amdgv_gpumon_metrics);

		if ((tstamp - adapt->pp.metrics_cache.tstamp < PP_METRICS_CACHE_EXPIRY_US)) {
			oss_memcpy(metrics, adapt->pp.metrics_cache.metrics, size);
			*event->data.gpumon_data.result = 0;
			break;
		}

		ret = adapt->gpumon.funcs->get_pp_metrics(adapt, metrics);
		*event->data.gpumon_data.result = ret;

		if (!ret) {
			oss_memcpy(adapt->pp.metrics_cache.metrics, metrics, size);
			adapt->pp.metrics_cache.tstamp = tstamp;
		}

		break;
	case GPUMON_GET_MAX_MCLK:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_max_mclk(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_MAX_SCLK:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_max_sclk(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_MAX_VCLK0:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_max_vclk0(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_MAX_VCLK1:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_max_vclk1(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_MAX_DCLK0:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_max_dclk0(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_MAX_DCLK1:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_max_dclk1(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_MIN_SCLK:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_min_sclk(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_MIN_MCLK:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_min_mclk(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_MIN_VCLK0:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_min_vclk0(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_MIN_VCLK1:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_min_vclk1(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_MIN_DCLK0:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_min_dclk0(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_MIN_DCLK1:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_min_dclk1(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_SCLK:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_sclk(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_ASIC_TEMP:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_asic_temperature(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_ALL_TEMP:
		temp = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_all_temperature(adapt, temp);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_GPU_POWER_USAGE:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_gpu_power_usage(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_GPU_POWER_CAP:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_gpu_power_capacity(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_SET_GPU_POWER_CAP:
		ret = adapt->gpumon.funcs->set_gpu_power_capacity(adapt,
			event->data.gpumon_data.val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_VDDC:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_vddc(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_DPM_STATUS:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_dpm_status(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_DPM_CAP:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_dpm_cap(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_GFX_ACT:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_gfx_activity(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_MEM_ACT:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_mem_activity(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_UVD_ACT:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_uvd_activity(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_VCE_ACT:
		val = event->data.gpumon_data.ptr;
		ret = adapt->gpumon.funcs->get_vce_activity(adapt, val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_FRU_SIGOUT:
		str = event->data.gpumon_data.ptr;
		ret = adapt->pp.pp_funcs->control_fru_sigout(adapt, str);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_RAS_EEPROM_CLEAR:
		ret = amdgv_umc_clean_bad_page_records(adapt);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_RAS_ERROR_INJECT:
		/* If necessary, verify the validity of the ras data */
		ret = amdgv_int_ras_trigger_error(adapt, event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_TURN_ON_ECC_INJECTION:
		ret = amdgv_control_fru_sigout(adapt, event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_RAS_REPORT:
		ret = adapt->gpumon.funcs->ras_report(adapt, event->data.gpumon_data.val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_ECC_INFO:
		ret = adapt->ecc.get_error_count(adapt, event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_CLEAN_CORRECTABLE_ERROR_COUNT:
		ret = amdgv_ecc_clean_correctable_error_count(adapt,
							      event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_ALLOC_VF:
		/* todo: need to check option using opt_type*/
		ret = amdgv_int_allocate_vf(adapt, event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_SET_VF_NUM:
		ret = amdgv_int_set_vf_number(adapt, event->data.gpumon_data.val);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_FREE_VF:
		ret = amdgv_int_free_vf(adapt, event->data.gpumon_data.val);
		if (ret == 0) {
			adapt->array_vf[event->data.gpumon_data.val].fb_offset_os = 0;
			adapt->array_vf[event->data.gpumon_data.val].fb_size_os = 0;
			adapt->array_vf[event->data.gpumon_data.val].fb_offset = 0;
			adapt->array_vf[event->data.gpumon_data.val].fb_size = 0;
			adapt->array_vf[event->data.gpumon_data.val].fb_offset_tmr = 0;
			adapt->array_vf[event->data.gpumon_data.val].fb_size_tmr = 0;
		}
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_SET_VF:
		ret = amdgv_vfmgr_set_vf(adapt, event->data.gpumon_data.vf.opt_type,
					 event->data.gpumon_data.vf.opt);
		*event->data.gpumon_data.result = ret;
		break;

	case GPUMON_GET_PCIE_CONFS:
		*event->data.gpumon_data.result = event->data.gpumon_data.pcie_confs.cb(
			event->data.gpumon_data.pcie_confs.dev,
			event->data.gpumon_data.pcie_confs.output.speed,
			event->data.gpumon_data.pcie_confs.output.width,
			event->data.gpumon_data.pcie_confs.output.max_vf);
		break;
	case GPUMON_GET_FW_ERR_RECORDS:
		ret = amdgv_get_fw_err_records(
			adapt, event->data.gpumon_data.fw_err_records.num_err_records,
			event->data.gpumon_data.fw_err_records.err_records);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_VF_FW_INFO:
		ret = amdgv_get_vf_fw_info(adapt, event->data.gpumon_data.fw_info.idx_vf,
					   event->data.gpumon_data.fw_info.num_fw,
					   event->data.gpumon_data.fw_info.fws);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_VRAM_INFO:
			ret = adapt->gpumon.funcs->get_vram_info(adapt, event->data.gpumon_data.ptr);
			*event->data.gpumon_data.result = ret;
			break;
	case GPUMON_IS_CLK_LOCKED:
		ret = adapt->gpumon.funcs->is_clk_locked(
			adapt, event->data.gpumon_data.clk_locked.clk_domain,
			event->data.gpumon_data.clk_locked.locked);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_SET_ACCELERATOR_PARTITION_PROFILE:
		ret = adapt->gpumon.funcs->set_accelerator_partition_profile(
			adapt, event->data.gpumon_data.ap.accelerator_partition_profile_index);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_SET_MEMORY_PARTITION_MODE:
		ret = adapt->gpumon.funcs->set_memory_partition_mode(
			adapt, event->data.gpumon_data.mp.memory_partition_mode);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_SET_SPATIAL_PARTITION_NUM:
		ret = adapt->gpumon.funcs->set_spatial_partition_num(
			adapt,
			event->data.gpumon_data.sp.spatial_partition_num);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_RESET_SPATIAL_PARTITION_NUM:
		ret = adapt->gpumon.funcs->reset_spatial_partition_num(adapt);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_ACCELERATOR_PARTITION_PROFILE:
		ret = adapt->gpumon.funcs->get_accelerator_partition_profile(
			adapt, event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_MEMORY_PARTITION_MODE:
		ret = adapt->gpumon.funcs->get_memory_partition_mode(
			adapt, event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_SPATIAL_PARTITION_NUM:
		ret = adapt->gpumon.funcs->get_spatial_partition_num(
			adapt, event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_PCIE_REPLAY_COUNT:
		ret = adapt->gpumon.funcs->get_pcie_replay_count(adapt,
				event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_CARD_FORM_FACTOR:
		ret = adapt->gpumon.funcs->get_card_form_factor(adapt,
				event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_MAX_CONFIG_POWER_LIMIT:
		ret = adapt->gpumon.funcs->get_max_configurable_power_limit(adapt,
				event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_DEFAULT_POWER_LIMIT:
		ret = adapt->gpumon.funcs->get_default_power_limit(adapt,
				event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_MIN_POWER_LIMIT:
		ret = adapt->gpumon.funcs->get_min_power_limit(adapt,
				event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_METRICS_EXT:
		ret = adapt->gpumon.funcs->get_metrics_ext(adapt,
				event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_NUM_METRICS_EXT_ENTRIES:
		ret = adapt->gpumon.funcs->get_num_metrics_ext_entries(adapt,
				event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_IS_POWER_MANAGEMENT_ENABLED:
		ret = adapt->gpumon.funcs->is_power_management_enabled(adapt,
				event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_LINK_METRICS:
		ret = adapt->gpumon.funcs->get_link_metrics(adapt,
				event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_LINK_TOPOLOGY:
		ret = adapt->gpumon.funcs->get_link_topology(adapt,
				event->data.gpumon_data.xgmi_node_topology.dest_adapt,
				event->data.gpumon_data.xgmi_node_topology.topology_info);
		*event->data.gpumon_data.result = ret;
		break;

	case GPUMON_GET_XGMI_FB_SHARING_CAPS:
		ret = adapt->gpumon.funcs->get_xgmi_fb_sharing_caps(adapt,
				event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_XGMI_FB_SHARING_MODE_INFO:
		ret = adapt->gpumon.funcs->get_xgmi_fb_sharing_mode_info(adapt,
				event->data.gpumon_data.xgmi_fb_sharing_mode_info.dest_adapt,
				event->data.gpumon_data.xgmi_fb_sharing_mode_info.mode,
				event->data.gpumon_data.xgmi_fb_sharing_mode_info.is_sharing_enabled);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_SET_XGMI_FB_SHARING_MODE:
		ret = adapt->gpumon.funcs->set_xgmi_fb_sharing_mode(adapt,
				event->data.gpumon_data.xgmi_set_fb_sharing_mode.mode);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_SET_XGMI_FB_SHARING_MODE_EX:
		ret = adapt->gpumon.funcs->set_xgmi_fb_sharing_mode_ex(adapt,
				event->data.gpumon_data.xgmi_set_fb_sharing_mode.mode,
				event->data.gpumon_data.xgmi_set_fb_sharing_mode.sharing_mask);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_SHUTDOWN_TEMP:
		ret = adapt->gpumon.funcs->get_shutdown_temperature(adapt,
				event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_GPU_CACHE_INFO:
		ret = adapt->gpumon.funcs->get_gpu_cache_info(adapt,
				event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_MAX_PCIE_LINK_GENERATION:
		ret = adapt->gpumon.funcs->get_max_pcie_link_generation(adapt,
				event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_RAS_TA_LOAD:
		ret = amdgv_int_ras_ta_load(adapt, event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_RAS_TA_UNLOAD:
		ret = amdgv_int_ras_ta_unload(adapt, event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_RESET_ALL_ERROR_COUNTS:
		ret = amdgv_ecc_reset_all_error_counts(adapt);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_FRU_PRODUCT_INFO:
		ret = adapt->pp.pp_funcs->get_fru_product_info(adapt);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_SET_PM_POLICY_LEVEL:
		ret = adapt->gpumon.funcs->set_pm_policy_level(adapt,
			event->data.gpumon_data.pm_info.p_type,
			event->data.gpumon_data.pm_info.level);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_DPM_POLICY_LEVEL:
		ret = adapt->gpumon.funcs->get_pm_policy(adapt,
			AMDGV_PP_PM_POLICY_SOC_PSTATE,
			event->data.gpumon_data.pm_policy.dpm_policy);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_XGMI_PLPD_POLICY_LEVEL:
		ret = adapt->gpumon.funcs->get_pm_policy(adapt,
			AMDGV_PP_PM_POLICY_XGMI_PLPD,
			event->data.gpumon_data.pm_policy.dpm_policy);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_CPER_GET_COUNT:
		if (!amdgv_device_is_gpu_lost(adapt))
			amdgv_mca_get_new_banks(adapt, AMDGV_MCA_ERROR_TYPE_CE);
		ret = amdgv_cper_get_count(adapt,
			event->data.gpumon_data.cper.get_count.rptr,
			event->data.gpumon_data.cper.get_count.wptr,
			event->data.gpumon_data.cper.get_count.avail_count,
			event->data.gpumon_data.cper.get_count.size);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_CPER_GET_ENTRIES:
		if (!amdgv_device_is_gpu_lost(adapt))
			amdgv_mca_get_new_banks(adapt, AMDGV_MCA_ERROR_TYPE_CE);
		ret = amdgv_cper_get_entries(adapt,
			event->data.gpumon_data.cper.get_entries.rptr,
			event->data.gpumon_data.cper.get_entries.buf,
			event->data.gpumon_data.cper.get_entries.buf_size,
			event->data.gpumon_data.cper.get_entries.write_count,
			event->data.gpumon_data.cper.get_entries.overflow_count,
			event->data.gpumon_data.cper.get_entries.left_size);
		*event->data.gpumon_data.result = ret;
		break;
	case GPUMON_GET_GFX_CONFIG:
		ret = adapt->gpumon.funcs->get_gfx_config(adapt, event->data.gpumon_data.ptr);
		*event->data.gpumon_data.result = ret;
		break;
	default:
		AMDGV_WARN("Unsupported GPU monitoring function %d\n",
			   event->data.gpumon_data.type);
		break;
	}

	return ret;
}

enum amdgv_xgmi_fb_sharing_mode amdgv_gpumon_xgmi_mode_map(
	enum amdgv_gpumon_xgmi_fb_sharing_mode gpumon_mode)
{
	switch (gpumon_mode) {
	case AMDGV_GPUMON_XGMI_FB_SHARING_MODE_1:
		return AMDGV_XGMI_FB_SHARING_MODE_1;
	case AMDGV_GPUMON_XGMI_FB_SHARING_MODE_2:
		return AMDGV_XGMI_FB_SHARING_MODE_2;
	case AMDGV_GPUMON_XGMI_FB_SHARING_MODE_4:
		return AMDGV_XGMI_FB_SHARING_MODE_4;
	case AMDGV_GPUMON_XGMI_FB_SHARING_MODE_8:
		return AMDGV_XGMI_FB_SHARING_MODE_8;
	case AMDGV_GPUMON_XGMI_FB_SHARING_MODE_CUSTOM:
		return AMDGV_XGMI_FB_SHARING_MODE_CUSTOM;
	default:
		return AMDGV_XGMI_FB_SHARING_MODE_UNKNOWN;
	}
}

int amdgv_gpumon_init_metrics_buf(struct amdgv_gpumon_metrics *metrics)
{
	oss_memset(metrics, AMDGV_GPUMON_METRIC_NOT_APPLICABLE,
		sizeof(struct amdgv_gpumon_metrics));

	return 0;
}

enum amdgv_live_info_status amdgv_gpumon_export_live_data(amdgv_dev_t dev, struct amdgv_live_info_gpumon *gpumon)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;

	gpumon->valid = adapt->product_info.valid;
	oss_memcpy(gpumon->model_number, adapt->product_info.model_number,
			STRLEN_NORMAL);
	oss_memcpy(gpumon->product_serial, adapt->product_info.product_serial,
			STRLEN_NORMAL);
	oss_memcpy(gpumon->fru_id, adapt->product_info.fru_id, STRLEN_NORMAL);

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

enum amdgv_live_info_status amdgv_gpumon_import_live_data(amdgv_dev_t dev, struct amdgv_live_info_gpumon *gpumon)
{
	struct amdgv_adapter *adapt = (struct amdgv_adapter *)dev;

	if (adapt->gpumon.funcs->get_vbios_cache) {
		adapt->gpumon.funcs->get_vbios_cache(adapt);
	} else {
		AMDGV_ERROR("get_vbios_cache function not implemented\n");
		return AMDGV_LIVE_INFO_STATUS_FEATURE_NOT_SUPPORTED;
	}

	adapt->product_info.visit = false;

	return AMDGV_LIVE_INFO_STATUS_SUCCESS;
}

int amdgv_gpumon_translate_fb_address(amdgv_dev_t dev,
	enum amdgv_gpumon_fb_addr_type src_type,
	enum amdgv_gpumon_fb_addr_type dest_type,
	union amdgv_gpumon_translate_fb_address src_addr,
	union amdgv_gpumon_translate_fb_address *dest_addr)
{
	struct amdgv_umc_fb_bank_addr  umc_bank_addr = { 0 };
	uint64_t soc_phy_addr;
	struct amdgv_adapter *adapt;
	int ret = 0;

	if (!dest_addr)
		return AMDGV_FAILURE;
	SET_ADAPT_AND_CHECK_STATUS(adapt, dev);

	/* Does not need to be queued as event as this is a SW translation */
	switch (src_type) {
	case AMDGV_GPUMON_FB_ADDR_SOC_PHY:
		soc_phy_addr = src_addr.soc_phy_addr;
		break;
	case AMDGV_GPUMON_FB_ADDR_BANK:
		umc_bank_addr.stack_id = src_addr.bank_addr.stack_id;
		umc_bank_addr.bank_group = src_addr.bank_addr.bank_group;
		umc_bank_addr.bank = src_addr.bank_addr.bank;
		umc_bank_addr.row = src_addr.bank_addr.row;
		umc_bank_addr.column = src_addr.bank_addr.column;
		umc_bank_addr.channel = src_addr.bank_addr.channel;
		umc_bank_addr.subchannel = src_addr.bank_addr.subchannel;
		ret = amdgv_umc_bank_to_soc_pa(adapt, umc_bank_addr, &soc_phy_addr);
		if (ret)
			return ret;
		break;
	case AMDGV_GPUMON_FB_ADDR_VF_PHY:
		if (src_addr.vf_phy_addr.idx_vf == AMDGV_PF_IDX ||
			!adapt->array_vf[src_addr.vf_phy_addr.idx_vf].configured)
			return AMDGV_FAILURE;
		ret = amdgv_umc_local_gpa_to_spa(adapt,
			src_addr.vf_phy_addr.addr,
			src_addr.vf_phy_addr.idx_vf,
			&soc_phy_addr);
		if (ret)
			return ret;
		break;
	default:
		return AMDGV_FAILURE;
	}

	switch (dest_type) {
	case AMDGV_GPUMON_FB_ADDR_SOC_PHY:
		dest_addr->soc_phy_addr = soc_phy_addr;
		break;
	case AMDGV_GPUMON_FB_ADDR_BANK:
		ret = amdgv_umc_soc_pa_to_bank(adapt, soc_phy_addr,
			&(umc_bank_addr));
		if (ret)
			return ret;
		dest_addr->bank_addr.stack_id = umc_bank_addr.stack_id;
		dest_addr->bank_addr.bank_group = umc_bank_addr.bank_group;
		dest_addr->bank_addr.bank = umc_bank_addr.bank;
		dest_addr->bank_addr.row = umc_bank_addr.row;
		dest_addr->bank_addr.column = umc_bank_addr.column;
		dest_addr->bank_addr.channel = umc_bank_addr.channel;
		dest_addr->bank_addr.subchannel = umc_bank_addr.subchannel;
		break;
	case AMDGV_GPUMON_FB_ADDR_VF_PHY:
		ret = amdgv_umc_local_spa_to_gpa(adapt, soc_phy_addr,
			&(dest_addr->vf_phy_addr.addr), &(dest_addr->vf_phy_addr.idx_vf));
		if (ret)
			return ret;
		break;
	default:
		return AMDGV_FAILURE;
	}
	return ret;
}
