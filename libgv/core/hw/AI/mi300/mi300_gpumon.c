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
 * THE SOFTWARE
 */
#include <amdgv.h>
#include <amdgv_device.h>
#include <amdgv_gpumon.h>
#include <amdgv_sched_internal.h>
#include <amdgv_oss_wrapper.h>
#include <amdgv_powerplay.h>
#include <amdgv_ip_discovery.h>
#include <amdgv_xgmi.h>
#include <amdgv_psp_gfx_if.h>
#include <amdgv_api_internal.h>

#include "mi300_powerplay.h"
#include "mi300_gpumon.h"
#include "mi300_nbio.h"
#include "mi300_xgmi.h"
#include "mi300_gfx.h"
#include "mi300_fru.h"
#include "mi300_psp.h"

#define MAX(a, b)	((a) > (b) ? (a) : (b))
#define adapt_to_smu(adapt)	      ((struct smu_context *)((adapt)->pp.smu_backend))

static const uint32_t this_block = AMDGV_MANAGEMENT_BLOCK;

/* Add hardcoded information to this table. */
struct mi300_gpumon_attribute_table {
	enum amdgv_gpumon_card_form_factor card_form_factor;
};

static struct mi300_gpumon_attribute_table mi300_attribute_table = {
	.card_form_factor = AMDGV_GPUMON_CARD_FORM_FACTOR__OAM,
};

static int mi300_get_asic_temperature(struct amdgv_adapter *adapt, int *val)
{
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0)
			*val = metrics.temp_hotspot;
		else
			*val = 0;
	}

	return ret;
}

int mi300_get_asic_temperature_mem(struct amdgv_adapter *adapt, int *val)
{
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0)
			*val = metrics.temp_mem;
		else
			*val = 0;
	}

	return ret;
}

int mi300_get_asic_temperature_hotspot(struct amdgv_adapter *adapt, int *val)
{
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0)
			*val = metrics.temp_hotspot;
		else
			*val = 0;
	}

	return ret;
}

static int mi300_get_sclk(struct amdgv_adapter *adapt, int *val)
{
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0)
			*val = metrics.clocks[AMDGV_PP_CLK_GFX].curr;
		else
			*val = 0;
	}

	return ret;
}

static int mi300_get_mem_activity(struct amdgv_adapter *adapt, int *val)
{
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0)
			*val = metrics.mem_usage;
		else
			*val = 0;
	}

	return ret;
}

int mi300_get_mm_activity(struct amdgv_adapter *adapt, int *val)
{
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0)
			*val = metrics.mm_usage;
		else
			*val = 0;
	}

	return ret;
}

static int mi300_get_gpu_power_usage(struct amdgv_adapter *adapt, int *val)
{
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0)
			*val = metrics.power;
		else
			*val = 0;
	}

	return ret;
}

static int mi300_get_gfx_activity(struct amdgv_adapter *adapt, int *val)
{
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0)
			*val = metrics.gfx_usage;
		else
			*val = 0;
	}

	return ret;
}

static int mi300_get_gecc(struct amdgv_adapter *adapt, uint32_t *enabled)
{
	*enabled = adapt->ecc.enabled;
	return 0;
}

static int mi300_get_ecc_info(struct amdgv_adapter *adapt, int *correctable_error,
			      int *uncorrectable_error)
{
	if (adapt->ecc.get_correctable_error_count)
		*correctable_error =
			adapt->ecc.get_correctable_error_count(adapt, AMDGV_PF_IDX);
	else
		*correctable_error = 0;

	if (adapt->ecc.get_uncorrectable_error_count)
		*uncorrectable_error =
			adapt->ecc.get_uncorrectable_error_count(adapt, AMDGV_PF_IDX);
	else
		*uncorrectable_error = 0;

	return 0;
}

static int mi300_get_vbios_cache(struct amdgv_adapter *adapt)
{
	struct amdgv_vbios_info *vbiosinfo = &adapt->vbios_cache;

	/* serial only needs to be init once */
	if (adapt->serial == 0) {
		if (adapt->gpumon.funcs->get_vbios_info) {
			vbiosinfo->serial = 0;
			adapt->gpumon.funcs->get_vbios_info(adapt, vbiosinfo);
			adapt->serial = vbiosinfo->serial;
		}
	}

	amdgv_vbios_cache_update(adapt);

	return 0;
}

static int mi300_get_vbios_info(struct amdgv_adapter *adapt,
				struct amdgv_vbios_info *vbiosinfo)
{
	uint32_t pci_data;
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0)
			vbiosinfo->serial = metrics.serial;
		else
			vbiosinfo->serial = 0;
	}
	oss_pci_read_config_dword(adapt->dev, 0, &pci_data);
	vbiosinfo->dev_id = pci_data >> 16;
	pci_data = 0;
	oss_pci_read_config_dword(adapt->dev, 8, &pci_data);
	vbiosinfo->rev_id = pci_data & 0x000000FF;

	return 0;
}

static int mi300_get_gpu_power_capacity(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_power_capacity) {
		ret = adapt->pp.pp_funcs->get_power_capacity(adapt, val);
		if (ret != 0)
			*val = 0;
	}

	return ret;
}

static int mi300_set_gpu_power_capacity(struct amdgv_adapter *adapt, int val)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->set_power_capacity)
		ret = adapt->pp.pp_funcs->set_power_capacity(adapt, val);

	return ret;
}

static int mi300_get_dpm_capacity(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_dpm_capacity) {
		ret = adapt->pp.pp_funcs->get_dpm_capacity(adapt, val);
		if (ret != 0)
			*val = 0;
	}

	return ret;
}

static int mi300_get_dpm_status(struct amdgv_adapter *adapt, int *val)
{
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0)
			*val = metrics.dpm;
		else
			*val = 0;
	}

	return ret;
}

static int mi300_get_max_sclk(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;
	uint32_t clk = 0;

	if (adapt->pp.pp_funcs->get_clock_limit) {
		ret = adapt->pp.pp_funcs->get_clock_limit(adapt, PP_CLOCK_TYPE__GFX,
							  PP_CLOCK_LIMIT_TYPE__SOFT_MAX, &clk);
		if (ret == 0)
			*val = (int)clk;
	}

	return ret;
}

static int mi300_get_max_mclk(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;
	uint32_t clk = 0;

	if (adapt->pp.pp_funcs->get_clock_limit) {
		ret = adapt->pp.pp_funcs->get_clock_limit(adapt, PP_CLOCK_TYPE__UCLK,
							  PP_CLOCK_LIMIT_TYPE__SOFT_MAX, &clk);
		if (ret == 0)
			*val = (int)clk;
	}

	return ret;
}

static int mi300_get_max_vclk0(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;
	uint32_t clk = 0;

	if (adapt->pp.pp_funcs->get_clock_limit) {
		ret = adapt->pp.pp_funcs->get_clock_limit(adapt,
					PP_CLOCK_TYPE__VCLK,
					PP_CLOCK_LIMIT_TYPE__SOFT_MAX,
					&clk);
		if (ret == 0)
			*val = (int)clk;
	}

	return ret;
}

static int mi300_get_max_vclk1(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;
	uint32_t clk = 0;

	if (adapt->pp.pp_funcs->get_clock_limit) {
		ret = adapt->pp.pp_funcs->get_clock_limit(adapt,
					PP_CLOCK_TYPE__VCLK_1,
					PP_CLOCK_LIMIT_TYPE__SOFT_MAX,
					&clk);
		if (ret == 0)
			*val = (int)clk;
	}

	return ret;
}

static int mi300_get_max_dclk0(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;
	uint32_t clk = 0;

	if (adapt->pp.pp_funcs->get_clock_limit) {
		ret = adapt->pp.pp_funcs->get_clock_limit(adapt,
					PP_CLOCK_TYPE__DCLK,
					PP_CLOCK_LIMIT_TYPE__SOFT_MAX,
					&clk);
		if (ret == 0)
			*val = (int)clk;
	}

	return ret;
}

static int mi300_get_max_dclk1(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;
	uint32_t clk = 0;

	if (adapt->pp.pp_funcs->get_clock_limit) {
		ret = adapt->pp.pp_funcs->get_clock_limit(adapt,
					PP_CLOCK_TYPE__DCLK_1,
					PP_CLOCK_LIMIT_TYPE__SOFT_MAX,
					&clk);
		if (ret == 0)
			*val = (int)clk;
	}

	return ret;
}

static int mi300_get_min_sclk(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;
	uint32_t clk = 0;

	if (adapt->pp.pp_funcs->get_clock_limit) {
		ret = adapt->pp.pp_funcs->get_clock_limit(adapt, PP_CLOCK_TYPE__GFX,
							  PP_CLOCK_LIMIT_TYPE__SOFT_MIN, &clk);
		if (ret == 0)
			*val = (int)clk;
	}

	return ret;
}

static int mi300_get_min_mclk(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;
	uint32_t clk = 0;

	if (adapt->pp.pp_funcs->get_clock_limit) {
		ret = adapt->pp.pp_funcs->get_clock_limit(adapt, PP_CLOCK_TYPE__UCLK,
							  PP_CLOCK_LIMIT_TYPE__SOFT_MIN, &clk);
		if (ret == 0)
			*val = (int)clk;
	}

	return ret;
}

static int mi300_get_min_vclk0(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;
	uint32_t clk = 0;

	if (adapt->pp.pp_funcs->get_clock_limit) {
		ret = adapt->pp.pp_funcs->get_clock_limit(adapt,
					PP_CLOCK_TYPE__VCLK,
					PP_CLOCK_LIMIT_TYPE__SOFT_MIN,
					&clk);
		if (ret == 0)
			*val = (int)clk;
	}

	return ret;
}

static int mi300_get_min_vclk1(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;
	uint32_t clk = 0;

	if (adapt->pp.pp_funcs->get_clock_limit) {
		ret = adapt->pp.pp_funcs->get_clock_limit(adapt,
					PP_CLOCK_TYPE__VCLK_1,
					PP_CLOCK_LIMIT_TYPE__SOFT_MIN,
					&clk);
		if (ret == 0)
			*val = (int)clk;
	}

	return ret;
}

static int mi300_get_min_dclk0(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;
	uint32_t clk = 0;

	if (adapt->pp.pp_funcs->get_clock_limit) {
		ret = adapt->pp.pp_funcs->get_clock_limit(adapt,
					PP_CLOCK_TYPE__DCLK,
					PP_CLOCK_LIMIT_TYPE__SOFT_MIN,
					&clk);
		if (ret == 0)
			*val = (int)clk;
	}

	return ret;
}

static int mi300_get_min_dclk1(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;
	uint32_t clk = 0;

	if (adapt->pp.pp_funcs->get_clock_limit) {
		ret = adapt->pp.pp_funcs->get_clock_limit(adapt,
					PP_CLOCK_TYPE__DCLK_1,
					PP_CLOCK_LIMIT_TYPE__SOFT_MIN,
					&clk);
		if (ret == 0)
			*val = (int)clk;
	}

	return ret;
}

static int mi300_get_pp_metrics(struct amdgv_adapter *adapt,
				struct amdgv_gpumon_metrics *metrics)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_pp_metrics)
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, metrics);

	return ret;
}

static int mi300_get_vram_info(struct amdgv_adapter *adapt,
			       struct amdgv_gpumon_vram_info *vram_info)
{
	vram_info->vram_size_mb = mi300_nbio_get_total_vram_size(adapt);
	vram_info->vram_type = adapt->vram_info.vram_type;
	vram_info->vram_vendor = AMDGV_GPUMON_VRAM_VENDOR__PLACEHOLDER0;
	vram_info->vram_bit_width = adapt->vram_info.vram_bit_width;

	return 0;
}

static int mi300_is_clk_locked(struct amdgv_adapter *adapt,
			       enum AMDGV_PP_CLK_DOMAIN clk_domain, uint8_t *clk_locked)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->is_clock_locked)
		ret = adapt->pp.pp_funcs->is_clock_locked(adapt, clk_domain, clk_locked);

	return ret;
}

static inline const char *mi300_get_memory_partition_mode_desc(
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

static const char *mi300_get_accelerator_partition_mode_desc(
	struct amdgv_adapter *adapt, uint32_t accelerator_partition_mode)
{
	switch (accelerator_partition_mode) {
	case 1:
		return "SPX";
	case 2:
		return "DPX";
	case 4:
		return (adapt->max_num_vf == 4) ? "CPX-4" : "QPX";
	case 8:
		return "CPX";
	default:
		return "UNKNOWN";
	}
}

static int mi300_get_memory_partition_config(
	struct amdgv_adapter *adapt,
	union amdgv_gpumon_memory_partition_config *memory_partition_config)
{
	if (adapt == NULL || memory_partition_config == NULL) {
		return AMDGV_ERROR_GPUMON_INVALID_OPTION;
	}

	memory_partition_config->mp_cap_mask = 0;
	memory_partition_config->mp_caps.nps1_cap = 1;
	memory_partition_config->mp_caps.nps4_cap = 1;

	return 0;
}

static int mi300_set_memory_partition_mode(
	struct amdgv_adapter *adapt,
	enum amdgv_memory_partition_mode memory_partition_mode)
{
	enum psp_status psp_ret = PSP_STATUS__SUCCESS;
	uint32_t default_accelerator_partition_mode;

	default_accelerator_partition_mode =
		mi300_nbio_get_accelerator_partition_mode_default_setting(
			adapt, memory_partition_mode);
	if (default_accelerator_partition_mode == 0) {
		AMDGV_ERROR("failed to get default accelerator_partition_mode. "
					"requested NPS%u mode is not supported\n",
					memory_partition_mode);
		return AMDGV_ERROR_GPUMON_INVALID_MODE;
	}

	if (mi300_nbio_is_partition_mode_combination_supported(adapt,
			memory_partition_mode,
			default_accelerator_partition_mode) == false) {
		AMDGV_ERROR("requested NPS%u mode is not supported\n",
			    memory_partition_mode);
		return AMDGV_ERROR_GPUMON_INVALID_MODE;
	}

	/* Request PSP to switch memory partition mode */
	psp_ret = mi300_psp_set_memory_partition_mode(adapt,
					memory_partition_mode);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		return AMDGV_FAILURE;
	}

	AMDGV_INFO("setting accelerator partition mode to %s for %uVF and %s\n",
				mi300_get_accelerator_partition_mode_desc(
					adapt, default_accelerator_partition_mode),
				adapt->num_vf,
				mi300_get_memory_partition_mode_desc(memory_partition_mode));

	/* Request PSP to switch compute partition mode */
	psp_ret = mi300_psp_set_accelerator_partition_mode(
		adapt, default_accelerator_partition_mode);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mi300_set_spatial_partition_num(struct amdgv_adapter *adapt,
					   uint32_t spatial_partition_num)
{
	return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
}

static struct amdgv_gpumon_accelerator_partition_profile_config
	mi300x_accelerator_partition_profile_configs = {
	8, // number_of_resource_profiles
	{
		{ 0, AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_XCC, 1, 1 },
		{ 1, AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_XCC, 2, 1 },
		{ 2, AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_XCC, 4, 1 },
		{ 3, AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_XCC, 8, 1 },
		{ 4, AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_DECODER, 1, 2 },
		{ 5, AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_DECODER, 1, 1 },
		{ 6, AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_DECODER, 2, 1 },
		{ 7, AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_DECODER, 4, 1 }
	},
	4, // number_of_profiles
	{
		{
			0,
			AMDGV_GPUMON_ACCELERATOR_PARTITION_SPX,
			{ .mp_caps = {.nps1_cap = 1 } },
			1,
			{0},
			2,
			{ { 3, 7 } },
			(1 << 1)
		},
		{
			1,
			AMDGV_GPUMON_ACCELERATOR_PARTITION_DPX,
			{ .mp_caps = {.nps1_cap = 1 } },
			2,
			{0, 1},
			2,
			{ { 2, 6 }, { 2, 6 } },
			(1 << 2),
		},
		{
			2,
			AMDGV_GPUMON_ACCELERATOR_PARTITION_QPX,
			{ .mp_caps = {.nps1_cap = 1, .nps4_cap = 1 } },
			4,
			{0, 1, 2, 3},
			2,
			{ { 1, 5 }, { 1, 5 }, { 1, 5 }, { 1, 5 } },
			(1 << 4)
		},
		{
			3,
			AMDGV_GPUMON_ACCELERATOR_PARTITION_CPX,
			{ .mp_caps = {.nps1_cap = 1, .nps4_cap = 1 } },
			8,
			{0, 1, 2, 3, 4, 5, 6, 7},
			2,
			{ { 0, 4 }, { 0, 4 }, { 0, 4 }, { 0, 4 }, { 0, 4 }, { 0, 4 }, { 0, 4 }, { 0, 4 } },
			(1 << 1) | (1 << 2) | (1 << 4) | (1 << 8)
		}
	}
};

static struct amdgv_gpumon_accelerator_partition_profile_config
	mi308x_accelerator_partition_profile_configs = {
	6, // number_of_resource_profiles
	{
		{ 0, AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_XCC, 1, 1 },
		{ 1, AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_XCC, 2, 1 },
		{ 2, AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_XCC, 4, 1 },
		{ 3, AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_DECODER, 1, 1 },
		{ 4, AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_DECODER, 2, 1 },
		{ 5, AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_DECODER, 4, 1 }
	},
	3, // number_of_profiles
	{
		{
			0,
			AMDGV_GPUMON_ACCELERATOR_PARTITION_SPX,
			{ .mp_caps = {.nps1_cap = 1 } },
			1,
			{0},
			2,
			{ { 2, 5 } },
			(1 << 1)
		},
		{
			1,
			AMDGV_GPUMON_ACCELERATOR_PARTITION_DPX,
			{ .mp_caps = {.nps1_cap = 1 } },
			2,
			{0, 1},
			2,
			{ { 1, 4 }, { 1, 4 } },
			(1 << 2)
		},
		{
			2,
			AMDGV_GPUMON_ACCELERATOR_PARTITION_CPX,
			{ .mp_caps = {.nps1_cap = 1, .nps4_cap = 1 } },
			4,
			{0, 1, 2, 3},
			2,
			{ { 0, 3 }, { 0, 3 }, { 0, 3 }, { 0, 3 } },
			(1 << 1) | (1 << 2) | (1 << 4)
		}
	}
};

/* Only containing profile configs valid for current num_vf, filled in
 * mi300_get_accelerator_partition_profile_asic_config
 */
static struct amdgv_gpumon_accelerator_partition_profile_config
	mi300x_accelerator_partition_profile_configs_valid = {
	0,
	{ { 0 } },
	0,
	{ { 0 } }
};

static struct amdgv_gpumon_accelerator_partition_profile_config *
mi300_get_accelerator_partition_profile_asic_config_global(struct amdgv_adapter *adapt)
{
	if (adapt->asic_type == CHIP_MI300X) {
		return &mi300x_accelerator_partition_profile_configs;
	} else if (adapt->asic_type == CHIP_MI308X) {
		return &mi308x_accelerator_partition_profile_configs;
	} else {
		AMDGV_ERROR("asic_type=%u not supported\n", adapt->asic_type);
		return NULL;
	}
}

static struct amdgv_gpumon_accelerator_partition_profile_config *
mi300_get_accelerator_partition_profile_asic_config(struct amdgv_adapter *adapt)
{
	struct amdgv_gpumon_accelerator_partition_profile_config
		*profile_asic_configs;
	struct amdgv_gpumon_accelerator_partition_profile_config
		*valid_profile_configs;
	uint32_t valid_profiles;
	uint32_t i;

	profile_asic_configs =
		mi300_get_accelerator_partition_profile_asic_config_global(adapt);
	if (profile_asic_configs == NULL) {
		return NULL;
	}

	valid_profile_configs = oss_zalloc(
		sizeof(struct amdgv_gpumon_accelerator_partition_profile_config));
	oss_memcpy(
		valid_profile_configs, profile_asic_configs,
		sizeof(struct amdgv_gpumon_accelerator_partition_profile_config));
	oss_memset(
		valid_profile_configs->profiles, 0,
		sizeof(struct amdgv_gpumon_acccelerator_partition_profile) *
		profile_asic_configs->number_of_profiles);
	valid_profiles = 0;
	for (i = 0; i < profile_asic_configs->number_of_profiles; i++) {
		if (profile_asic_configs->profiles[i].support_vf_num & (1 << adapt->num_vf)) {
			valid_profile_configs->profiles[valid_profiles] =
				profile_asic_configs->profiles[i];
			valid_profile_configs->profiles[valid_profiles].profile_index =
				valid_profiles;
			valid_profiles++;
		}
	}
	valid_profile_configs->number_of_profiles = valid_profiles;
	oss_memcpy(
		&mi300x_accelerator_partition_profile_configs_valid, valid_profile_configs,
		sizeof(struct amdgv_gpumon_accelerator_partition_profile_config));
	oss_free(valid_profile_configs);

	return &mi300x_accelerator_partition_profile_configs_valid;
}

static int mi300_get_accelerator_partition_profile_config_global(
	struct amdgv_adapter *adapt,
	struct amdgv_gpumon_accelerator_partition_profile_config
		*profile_configs)
{
	struct amdgv_gpumon_accelerator_partition_profile_config
		*profile_asic_configs;

	if (adapt == NULL || profile_configs == NULL) {
		return AMDGV_ERROR_GPUMON_INVALID_OPTION;
	}

	profile_asic_configs =
		mi300_get_accelerator_partition_profile_asic_config_global(adapt);
	if (profile_asic_configs == NULL) {
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	}

	oss_memcpy(
		profile_configs, profile_asic_configs,
		sizeof(struct amdgv_gpumon_accelerator_partition_profile_config));

	return 0;
}

static int mi300_get_accelerator_partition_profile_config(
	struct amdgv_adapter *adapt,
	struct amdgv_gpumon_accelerator_partition_profile_config
		*profile_configs)
{
	struct amdgv_gpumon_accelerator_partition_profile_config
		*profile_asic_configs;

	if (adapt == NULL || profile_configs == NULL) {
		return AMDGV_ERROR_GPUMON_INVALID_OPTION;
	}

	profile_asic_configs =
		mi300_get_accelerator_partition_profile_asic_config(adapt);
	if (profile_asic_configs == NULL) {
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	}

	oss_memcpy(
		profile_configs, profile_asic_configs,
		sizeof(struct amdgv_gpumon_accelerator_partition_profile_config));

	return 0;
}

static uint32_t mi300_get_accelerator_partition_mode(struct amdgv_adapter *adapt,
					       uint32_t profile_index)
{
	struct amdgv_gpumon_accelerator_partition_profile_config
		*profile_asic_configs;
	uint32_t i;

	profile_asic_configs =
		mi300_get_accelerator_partition_profile_asic_config(adapt);
	if (profile_asic_configs == NULL) {
		return 0;
	}

	for (i = 0; i < profile_asic_configs->number_of_profiles; i++) {
		if (profile_asic_configs->profiles[i].profile_index == profile_index) {
			return profile_asic_configs->profiles[i].num_partitions;
		}
	}

	return 0;
}

static int mi300_set_accelerator_partition_profile(struct amdgv_adapter *adapt,
						   uint32_t profile_index)
{
	int ret;
	enum psp_status psp_ret = PSP_STATUS__SUCCESS;
	uint32_t req_accelerator_partition_mode;
	enum amdgv_memory_partition_mode curr_memory_partition_mode;

	req_accelerator_partition_mode =
		mi300_get_accelerator_partition_mode(adapt, profile_index);
	if (!req_accelerator_partition_mode) {
		AMDGV_ERROR(
			"requested accelerator_partition_profile_index=%u is not supported\n",
			profile_index);
		return AMDGV_FAILURE;
	}

	ret = mi300_nbio_get_curr_memory_partition_mode(
		adapt, &curr_memory_partition_mode);
	if (ret || (curr_memory_partition_mode != adapt->mcp.memory_partition_mode)) {
		AMDGV_ERROR("failed to get current memory partition mode or memory partition mode mismatch\n");
		return AMDGV_FAILURE;
	}

	/* Requested accelerator partitions should not be less than memory partitions
	 * for example, if the requested compute mode is DPX, but current NPS mode is NPS4,
	 * host driver should fail the request with indication that DPX is not supported with NPS4.
	 */
	if (mi300_nbio_is_partition_mode_combination_supported(adapt,
			curr_memory_partition_mode,
			req_accelerator_partition_mode) == false) {
		AMDGV_ERROR(
			"requested accelerator_partition_mode=%s is not supported with %s mode and %uVF\n",
			mi300_get_accelerator_partition_mode_desc(
				adapt, req_accelerator_partition_mode),
			mi300_get_memory_partition_mode_desc(
				curr_memory_partition_mode),
			adapt->num_vf);
		return AMDGV_ERROR_GPUMON_INVALID_MODE;
	}

	/* Request PSP to switch compute partition mode */
	psp_ret = mi300_psp_set_accelerator_partition_mode(
		adapt, req_accelerator_partition_mode);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mi300_get_accelerator_partition_profile(
	struct amdgv_adapter *adapt,
	struct amdgv_gpumon_acccelerator_partition_profile *profile)
{
	uint32_t accelerator_partition_mode;
	struct amdgv_gpumon_accelerator_partition_profile_config
		*profile_asic_configs;
	uint32_t i;

	if (adapt == NULL || profile == NULL) {
		return AMDGV_ERROR_GPUMON_INVALID_OPTION;
	}

	profile_asic_configs =
		mi300_get_accelerator_partition_profile_asic_config(adapt);
	if (profile_asic_configs == NULL) {
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	}

	accelerator_partition_mode = mi300_nbio_get_accelerator_partition_mode(adapt);
	if (!accelerator_partition_mode) {
		AMDGV_ERROR("failed to get current accelerator_partition_mode\n");
		return AMDGV_FAILURE;
	}

	for (i = 0; i < profile_asic_configs->number_of_profiles; i++) {
		if (profile_asic_configs->profiles[i].num_partitions == accelerator_partition_mode) {
			*profile = profile_asic_configs->profiles[i];
			break;
		}
	}
	if (i == profile_asic_configs->number_of_profiles) {
		AMDGV_ERROR("no cp profile found with accelerator_partition_mode=%u\n",
			accelerator_partition_mode);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mi300_reset_spatial_partition_num(struct amdgv_adapter *adapt)
{
	return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
}

static int mi300_get_spatial_partition_caps(
	struct amdgv_adapter *adapt,
	struct amdgv_gpumon_spatial_partition_caps *spatial_partition_caps)
{
	if (adapt == NULL || spatial_partition_caps == NULL) {
		return AMDGV_ERROR_GPUMON_INVALID_OPTION;
	}

	spatial_partition_caps->num_xcc = adapt->mcp.gfx.num_xcc;
	spatial_partition_caps->num_sdma = adapt->sdma.num_instances;
	spatial_partition_caps->num_vcn = 4;
	spatial_partition_caps->num_jpeg = 8;

	return 0;
}

static int mi300_get_memory_partition_mode(
	struct amdgv_adapter *adapt,
	struct amdgv_gpumon_memory_partition_info *memory_partition_info)
{
	uint32_t i;
	int ret;

	if (adapt == NULL || memory_partition_info == NULL) {
		return AMDGV_ERROR_GPUMON_INVALID_OPTION;
	}

	ret = mi300_nbio_get_curr_memory_partition_mode(adapt,
			&memory_partition_info->memory_partition_mode);
	if (ret) {
		return ret;
	}

	memory_partition_info->num_numa_ranges = adapt->mcp.numa_count;
	for (i = 0; i < memory_partition_info->num_numa_ranges; i++) {
		memory_partition_info->numa_range[i].memory_type = AMDGV_GPUMON_DGPU_VRAM_TYPE__HBM3;
		memory_partition_info->numa_range[i].start = adapt->mcp.numa_range[i].start;
		memory_partition_info->numa_range[i].end = adapt->mcp.numa_range[i].end;
	}

	return 0;
}

static int mi300_get_spatial_partition_num(struct amdgv_adapter *adapt,
					   uint32_t *spatial_partition_num)
{
	*spatial_partition_num = adapt->sched.num_spatial_partitions;

	return 0;
}

static int mi300_get_shutdown_temperature(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_shutdown_temperature) {
		ret = adapt->pp.pp_funcs->get_shutdown_temperature(adapt, val);
	}

	return ret;
}

static int mi300_get_pcie_replay_count(struct amdgv_adapter *adapt, int *val)
{
	*val = mi300_nbio_get_pcie_replay_count(adapt);

	return 0;
}

static int mi300_get_card_form_factor(struct amdgv_adapter *adapt,
				      enum amdgv_gpumon_card_form_factor *card_form_factor)
{
	*card_form_factor = mi300_attribute_table.card_form_factor;

	return 0;
}

static int mi300_get_max_configurable_power_limit(struct amdgv_adapter *adapt,
						  int *power_limit)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_max_configurable_power_limit) {
		ret = adapt->pp.pp_funcs->get_max_configurable_power_limit(adapt, power_limit);
	}

	return ret;
}

static int mi300_get_default_power_limit(struct amdgv_adapter *adapt,
						  int *default_power)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_default_power_limit) {
		ret = adapt->pp.pp_funcs->get_default_power_limit(adapt, default_power);
	}

	return ret;
}

/* PMFW allows to set power to zero so the min limit is hardcoded */
static int mi300_get_min_power_limit(struct amdgv_adapter *adapt,
				int *val)
{
	*val = 0;

	return 0;
}

static int mi300_get_metrics_ext(struct amdgv_adapter *adapt,
				struct amdgv_gpumon_metrics_ext *metrics_ext)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_metrics_ext) {
		ret = adapt->pp.pp_funcs->get_metrics_ext(adapt, metrics_ext);
	}

	return ret;
}

static int mi300_get_num_metrics_ext_entries(struct amdgv_adapter *adapt,
				uint32_t *entries)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_num_metrics_ext_entries) {
		ret = adapt->pp.pp_funcs->get_num_metrics_ext_entries(adapt, entries);
	}

	return ret;
}

static int mi300_is_pm_enabled(struct amdgv_adapter *adapt, bool *pm_enabled)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->is_pm_enabled) {
		ret = adapt->pp.pp_funcs->is_pm_enabled(adapt, pm_enabled);
	}

	return ret;
}

static int mi300_gpumon_smu_get_pm_policy(struct amdgv_adapter *adapt,
			enum amdgv_pp_pm_policy p_type,
			struct amdgv_gpumon_smu_dpm_policy *policy)
{
	int ret = AMDGV_FAILURE;
	struct mi300_smu_dpm_policy *policy_int;
	uint32_t i = 0;

	ret = mi300_smu_get_pm_policy(adapt, p_type, &policy_int);
	if (ret)
		return ret;

	policy->current_level = policy_int->current_level;
	policy->policy_type = policy_int->policy_type;
	for (i = 0; i < policy_int->num_supported; i++) {
		policy->policies[i].policy_id = policy_int->policies[i].policy_id;
		policy->policies[i].policy_description = policy_int->policies[i].policy_description;
	}

	return 0;
}

static int mi300_gpumon_smu_set_pm_policy_level(struct amdgv_adapter *adapt,
			enum amdgv_pp_pm_policy p_type,
			enum amdgv_pp_policy_soc_pstate level)
{
	return mi300_smu_compare_and_set_pm_policy(adapt, p_type, level);
}

static int mi300_get_link_metrics(struct amdgv_adapter *adapt,
				  struct amdgv_gpumon_link_metrics *link_metrics)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_link_metrics) {
		ret = adapt->pp.pp_funcs->get_link_metrics(adapt, link_metrics);
	}

	return ret;
}

#define MI300_XGMI_LINK_WEIGHT 15
#define MI300_PCIE_LINK_WEIGHT 20

static int mi300_get_link_topology(struct amdgv_adapter *adapt,
				   struct amdgv_adapter *dest_adapt,
				   struct amdgv_gpumon_link_topology_info *topology_info)
{
	int i;
	struct amdgv_hive_info *hive;
	struct amdgv_xgmi_psp_topology_info *psp_topology_info = NULL;
	struct amdgv_xgmi_psp_node_info *psp_node_info = NULL;

	/* TODO: Certain CPUs may disable Peer2Peer. Need to verify whether device is
	 * accessible */
	topology_info->link_status = AMDGV_GPUMON_LINK_STATUS_ENABLED;

	/* N/A on MI300X */
	if (dest_adapt == adapt) {
		topology_info->num_hops = 0;
		topology_info->weight = 0;
		topology_info->is_fb_sharing_enabled = true;
		topology_info->link_type = AMDGV_GPUMON_LINK_TYPE_NOT_APPLICABLE;
		// Set p2p_caps for XGMI
		topology_info->p2p_caps.is_iolink_coherent = true;
		topology_info->p2p_caps.is_iolink_atomics_32bit =
			(adapt->pcie_atomic_ops_support_flags &
			PCIE_DEVICE_CAP2__ATOMIC_COMP32) ? 1 : 0;
		topology_info->p2p_caps.is_iolink_atomics_64bit =
			(adapt->pcie_atomic_ops_support_flags &
			PCIE_DEVICE_CAP2__ATOMIC_COMP64) ? 1 : 0;
		topology_info->p2p_caps.is_iolink_dma = true;
		topology_info->p2p_caps.is_iolink_bi_directional = true;
		return 0;
	}

	topology_info->is_fb_sharing_enabled = false;

	hive = amdgv_get_xgmi_hive(adapt);

	if (hive && amdgv_xgmi_is_node_in_hive(hive, dest_adapt)) {

		topology_info->link_type = AMDGV_GPUMON_LINK_TYPE_XGMI3;

		/*
		* In case previous update Topology failed, we cannot use the cached data.
		* User needs latest status to recover to new topology. Query XGMI TA each time.
		*/
		psp_topology_info = &adapt->xgmi.topology_info;

		if (amdgv_psp_xgmi_get_topology_info(adapt, hive, psp_topology_info)) {
			topology_info->link_status = AMDGV_GPUMON_LINK_STATUS_ERROR;
			return 0;
		}

		for (i = 0; i < psp_topology_info->num_nodes; i++) {
			if (psp_topology_info->node[i].node_id == dest_adapt->xgmi.node_id) {
				psp_node_info = &(psp_topology_info->node[i]);
				break;
			}
		}

		if (!psp_node_info) {
			return AMDGV_FAILURE;
		}

		topology_info->is_fb_sharing_enabled = psp_node_info->is_sharing_enabled;
		topology_info->num_hops = psp_node_info->num_hops;
		topology_info->weight = MI300_XGMI_LINK_WEIGHT * topology_info->num_hops;

		// Set p2p_caps for XGMI
		topology_info->p2p_caps.is_iolink_coherent = true;
		topology_info->p2p_caps.is_iolink_atomics_32bit =
			(adapt->pcie_atomic_ops_support_flags &
			PCIE_DEVICE_CAP2__ATOMIC_COMP32) ? 1 : 0;
		topology_info->p2p_caps.is_iolink_atomics_64bit =
			(adapt->pcie_atomic_ops_support_flags &
			PCIE_DEVICE_CAP2__ATOMIC_COMP64) ? 1 : 0;
		topology_info->p2p_caps.is_iolink_dma = true;
		topology_info->p2p_caps.is_iolink_bi_directional = true;
	}

	/* If XGMI FB Sharing is disabled / N/A, fallback on PCIE connection (not POR for MI300) */
	if (!topology_info->is_fb_sharing_enabled) {
		if (oss_get_device_numa_node(adapt->dev) == oss_get_device_numa_node(dest_adapt->dev)) {
			topology_info->num_hops = 2;
		}
		else {
			topology_info->num_hops = 3;
		}

		topology_info->link_type = AMDGV_GPUMON_LINK_TYPE_PCIE;
		topology_info->weight = MI300_PCIE_LINK_WEIGHT * topology_info->num_hops;

		// Set p2p_caps for PCIe
		topology_info->p2p_caps.is_iolink_coherent = false;
		topology_info->p2p_caps.is_iolink_atomics_32bit =
			(adapt->pcie_atomic_ops_support_flags &
			PCIE_DEVICE_CAP2__ATOMIC_COMP32) ? 1 : 0;
		topology_info->p2p_caps.is_iolink_atomics_64bit =
			(adapt->pcie_atomic_ops_support_flags &
			PCIE_DEVICE_CAP2__ATOMIC_COMP64) ? 1 : 0;
		topology_info->p2p_caps.is_iolink_dma = true;
		topology_info->p2p_caps.is_iolink_bi_directional = true;
	}

	return 0;
}

static int mi300_get_xgmi_fb_sharing_caps(struct amdgv_adapter *adapt,
					  union amdgv_gpumon_xgmi_fb_sharing_caps *caps)
{
	caps->xgmi_fb_sharing_cap_mask = 0x0;

	if (adapt->xgmi.phy_nodes_num >= 2) {
		caps->mode_1_cap = true;
		caps->mode_2_cap = true;
		caps->mode_custom_cap = true;
	}

	if (adapt->xgmi.phy_nodes_num >= 4) {
		caps->mode_4_cap = true;
	}

	if (adapt->xgmi.phy_nodes_num >= 8) {
		caps->mode_8_cap = true;
	}
	return 0;
}

static int mi300_get_xgmi_fb_sharing_mode_info(struct amdgv_adapter *src_adapt,
					       struct amdgv_adapter *dest_adapt,
					       enum amdgv_gpumon_xgmi_fb_sharing_mode mode,
					       uint8_t *is_sharing_enabled)
{
	struct amdgv_hive_info *hive;
	enum amdgv_xgmi_fb_sharing_mode libgv_mode;

	libgv_mode = amdgv_gpumon_xgmi_mode_map(mode);
	if (libgv_mode > MI300_XGMI_MAX_SUPPORTED_MODE)
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;

	if (libgv_mode == AMDGV_XGMI_FB_SHARING_MODE_CUSTOM) {
		/* Return the same result as mode 4 for custom sharing mode
		 * When query sharing mode info for custom mode, return true for nodes
		 * inside a cpu numa node, which is in current setup the same as mode 4
		 */
		libgv_mode = AMDGV_XGMI_FB_SHARING_MODE_4;
	}

	hive = amdgv_get_xgmi_hive(src_adapt);

	if (hive && amdgv_xgmi_is_node_in_hive(hive, dest_adapt)) {
		*is_sharing_enabled = amdgv_xgmi_is_fb_sharing_allowed(
			src_adapt, src_adapt->xgmi.phy_node_id, dest_adapt->xgmi.phy_node_id,
			libgv_mode);
	} else {
		*is_sharing_enabled = false;
	}

	return 0;
}

static int save_fb_sharing_mode(void *context)
{
	struct amdgv_adapter *adapt;

	SET_ADAPT_AND_CHECK_STATUS(adapt, context);

	oss_save_fb_sharing_mode(adapt->dev, adapt->xgmi.fb_sharing_mode);

	return 0;
}

static int
mi300_set_xgmi_fb_sharing_mode(struct amdgv_adapter *adapt,
			enum amdgv_gpumon_xgmi_fb_sharing_mode mode)
{
	int ret = AMDGV_FAILURE;
	enum amdgv_xgmi_fb_sharing_mode libgv_mode;

	libgv_mode = amdgv_gpumon_xgmi_mode_map(mode);
	if (libgv_mode > MI300_XGMI_MAX_SUPPORTED_MODE)
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;

	ret = amdgv_xgmi_update_topology_with_fb_sharing_mode(adapt, libgv_mode);

	/* Must be done in passive IRQL. Add job to workqueue. */
	oss_schedule_work(adapt->dev, save_fb_sharing_mode, (void *)adapt);

	return ret;
}

static int mi300_set_xgmi_fb_sharing_mode_ex(struct amdgv_adapter *adapt,
			enum amdgv_gpumon_xgmi_fb_sharing_mode mode, uint32_t sharing_mask)
{
	int ret = AMDGV_FAILURE;
	enum amdgv_xgmi_fb_sharing_mode libgv_mode;

	libgv_mode = amdgv_gpumon_xgmi_mode_map(mode);
	if (libgv_mode > MI300_XGMI_MAX_SUPPORTED_MODE)
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;

	if (libgv_mode == AMDGV_XGMI_FB_SHARING_MODE_CUSTOM)
		adapt->xgmi.custom_mode_sharing_mask = sharing_mask;
	else
		adapt->xgmi.custom_mode_sharing_mask = 0;


	ret = amdgv_xgmi_update_topology_with_fb_sharing_mode(adapt, libgv_mode);

	/* Must be done in passive IRQL. Add job to workqueue. */
	oss_schedule_work(adapt->dev, save_fb_sharing_mode, (void *)adapt);

	return ret;
}

/* @TODO: Move this into MI300 UMC backend once it is created (pending RAS) */
/* number of umc instance with memory map register access */
#define MI300_UMC_INSTANCE_NUM		16

/* MI300 IP Discovery does not have this information so we must Hardcode it */
static int mi300_get_gpu_cache_info(struct amdgv_adapter *adapt,
			struct amdgv_gpumon_gpu_cache_info *gpu_cache_info)
{
	struct amdgv_ip_discovery_info *pf_copy;
	struct amdgv_gc_info_v2_1 *gc_info;
	struct amdgv_mall_info_v2_0 *mall_info;
	uint32_t i, xcc_max_num_cu = 0;

	pf_copy = &adapt->ip_discovery.pf_copy;

	if (pf_copy->gchdr->version_major != 2 || pf_copy->gchdr->version_minor != 1) {
		AMDGV_WARN("Unsupported GC version!\n");
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;
	}

	gc_info = GET_GC_TABLE_V2_1(pf_copy->gchdr);
	mall_info = GET_MALL_INFO_V2_0(pf_copy->mhdr);

	gpu_cache_info->num_cache_types = 5;

	if (gpu_cache_info->num_cache_types > AMDGV_GPUMON_MAX_CACHE_TYPES)
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;

	if (adapt->asic_type != CHIP_MI300X && adapt->asic_type != CHIP_MI308X
			 )
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;

	/* vL1D (Also known as TCP, GL0, CU$). On MI300 marked as GL1 cache */
	gpu_cache_info->cache[0].flags = (AMDGV_GPUMON_CACHE_FLAGS_ENABLED |
				AMDGV_GPUMON_CACHE_FLAGS_DATA_CACHE |
				AMDGV_GPUMON_CACHE_FLAGS_SIMD_CACHE);
	gpu_cache_info->cache[0].cache_level = 1;
	gpu_cache_info->cache[0].max_num_cu_shared =
		gc_info->gc_num_cu_per_sh / gc_info->gc_num_tcp_per_sh;
	gpu_cache_info->cache[0].cache_size_kb =
		gc_info->gc_tcp_size_per_cu * gpu_cache_info->cache[0].max_num_cu_shared;
	gpu_cache_info->cache[0].num_cache_instance =
		adapt->config.gfx.active_cu_count / gpu_cache_info->cache[0].max_num_cu_shared;

	/* Scalar L1 data cache (sL1D or K$) */
	gpu_cache_info->cache[1].flags = (AMDGV_GPUMON_CACHE_FLAGS_ENABLED |
				AMDGV_GPUMON_CACHE_FLAGS_DATA_CACHE |
				AMDGV_GPUMON_CACHE_FLAGS_SIMD_CACHE);
	gpu_cache_info->cache[1].cache_level = 1;
	gpu_cache_info->cache[1].cache_size_kb = gc_info->gc_scalar_data_cache_size_per_sqc;
	gpu_cache_info->cache[1].max_num_cu_shared = gc_info->gc_num_cu_per_sqc;
	gpu_cache_info->cache[1].num_cache_instance =
		adapt->config.gfx.active_cu_count / gc_info->gc_num_cu_per_sqc;

	/* Sequencer Cache – Instruction cache (I$) */
	gpu_cache_info->cache[2].flags = (AMDGV_GPUMON_CACHE_FLAGS_ENABLED |
				AMDGV_GPUMON_CACHE_FLAGS_INST_CACHE |
				AMDGV_GPUMON_CACHE_FLAGS_SIMD_CACHE);
	gpu_cache_info->cache[2].cache_level = 1;
	gpu_cache_info->cache[2].cache_size_kb = gc_info->gc_instruction_cache_size_per_sqc;
	gpu_cache_info->cache[2].max_num_cu_shared = gc_info->gc_num_cu_per_sqc;
	gpu_cache_info->cache[2].num_cache_instance = adapt->config.gfx.active_cu_count / gc_info->gc_num_cu_per_sqc;

	for (i = 0; i < adapt->mcp.gfx.num_xcc; i++)
		xcc_max_num_cu = MAX (xcc_max_num_cu, mi300_gfx_get_xcc_cu_count(adapt, i));

	/* Texture Cache per Channel  (GL2, SE$)*/
	gpu_cache_info->cache[3].flags = (AMDGV_GPUMON_CACHE_FLAGS_ENABLED |
				AMDGV_GPUMON_CACHE_FLAGS_DATA_CACHE |
				AMDGV_GPUMON_CACHE_FLAGS_SIMD_CACHE);
	gpu_cache_info->cache[3].cache_level = 2;
	gpu_cache_info->cache[3].max_num_cu_shared = xcc_max_num_cu;
	gpu_cache_info->cache[3].cache_size_kb = gc_info->gc_tcc_size;
	gpu_cache_info->cache[3].num_cache_instance = adapt->mcp.gfx.num_xcc;

	/* Memory Attached Last Level Cache  (Mall, L3 Cache)*/
	gpu_cache_info->cache[4].flags = (AMDGV_GPUMON_CACHE_FLAGS_ENABLED |
				AMDGV_GPUMON_CACHE_FLAGS_DATA_CACHE |
				AMDGV_GPUMON_CACHE_FLAGS_SIMD_CACHE);
	gpu_cache_info->cache[4].cache_level = 3;
	gpu_cache_info->cache[4].max_num_cu_shared = xcc_max_num_cu * adapt->mcp.gfx.num_xcc;
	gpu_cache_info->cache[4].cache_size_kb =
		((MI300_UMC_INSTANCE_NUM * mall_info->mall_size_per_umc) / 1024);
	gpu_cache_info->cache[4].num_cache_instance = 1;

	return 0;
}

static int mi300_get_max_pcie_link_generation(struct amdgv_adapter *adapt,
			int *val)
{
	*val = 5;

	return 0;
}

static int mi300_get_gfx_config(struct amdgv_adapter *adapt,
			struct amdgv_gpumon_gfx_config *config)
{
	config->max_shader_engines = adapt->config.gfx.max_shader_engines;
	config->max_cu_per_sh = adapt->config.gfx.max_cu_per_sh;
	config->max_sh_per_se = adapt->config.gfx.max_sh_per_se;
	config->max_waves_per_simd = adapt->config.gfx.max_waves_per_simd;
	config->wave_size = adapt->config.gfx.wave_size;
	config->active_cu_count = adapt->config.gfx.active_cu_count;
	config->major = adapt->config.gfx.major;
	config->minor = adapt->config.gfx.minor;

	return 0;
}

static const struct amdgv_gpumon_funcs mi300_gpumon_funcs = {
	.get_asic_temperature = mi300_get_asic_temperature,
	.get_gpu_power_usage = mi300_get_gpu_power_usage,
	.get_gpu_power_capacity = mi300_get_gpu_power_capacity,
	.set_gpu_power_capacity = mi300_set_gpu_power_capacity,
	.get_dpm_cap = mi300_get_dpm_capacity,
	.get_dpm_status = mi300_get_dpm_status,
	.get_sclk = mi300_get_sclk,
	.get_max_sclk = mi300_get_max_sclk,
	.get_max_mclk = mi300_get_max_mclk,
	.get_max_vclk0 = mi300_get_max_vclk0,
	.get_max_vclk1 = mi300_get_max_vclk1,
	.get_max_dclk0 = mi300_get_max_dclk0,
	.get_max_dclk1 = mi300_get_max_dclk1,
	.get_min_sclk = mi300_get_min_sclk,
	.get_min_mclk = mi300_get_min_mclk,
	.get_min_vclk0 = mi300_get_min_vclk0,
	.get_min_vclk1 = mi300_get_min_vclk1,
	.get_min_dclk0 = mi300_get_min_dclk0,
	.get_min_dclk1 = mi300_get_min_dclk1,
	.get_gfx_activity = mi300_get_gfx_activity,
	.get_mem_activity = mi300_get_mem_activity,
	.get_gecc = mi300_get_gecc,
	.get_ecc_info = mi300_get_ecc_info,
	.get_pp_metrics = mi300_get_pp_metrics,
	.get_vbios_info = mi300_get_vbios_info,
	.get_vbios_cache = mi300_get_vbios_cache,
	.get_vram_info = mi300_get_vram_info,
	.is_clk_locked = mi300_is_clk_locked,
	.get_accelerator_partition_profile_config_global = mi300_get_accelerator_partition_profile_config_global,
	.get_accelerator_partition_profile_config = mi300_get_accelerator_partition_profile_config,
	.set_accelerator_partition_profile = mi300_set_accelerator_partition_profile,
	.get_accelerator_partition_profile = mi300_get_accelerator_partition_profile,
	.get_memory_partition_config = mi300_get_memory_partition_config,
	.get_spatial_partition_caps = mi300_get_spatial_partition_caps,
	.set_memory_partition_mode = mi300_set_memory_partition_mode,
	.set_spatial_partition_num = mi300_set_spatial_partition_num,
	.reset_spatial_partition_num = mi300_reset_spatial_partition_num,
	.get_memory_partition_mode = mi300_get_memory_partition_mode,
	.get_spatial_partition_num = mi300_get_spatial_partition_num,
	.get_pcie_replay_count = mi300_get_pcie_replay_count,
	.get_card_form_factor = mi300_get_card_form_factor,
	.get_max_configurable_power_limit = mi300_get_max_configurable_power_limit,
	.get_default_power_limit = mi300_get_default_power_limit,
	.get_min_power_limit = mi300_get_min_power_limit,
	.get_metrics_ext = mi300_get_metrics_ext,
	.get_num_metrics_ext_entries = mi300_get_num_metrics_ext_entries,
	.is_power_management_enabled = mi300_is_pm_enabled,
	.get_link_metrics = mi300_get_link_metrics,
	.get_link_topology = mi300_get_link_topology,
	.get_xgmi_fb_sharing_caps = mi300_get_xgmi_fb_sharing_caps,
	.get_xgmi_fb_sharing_mode_info = mi300_get_xgmi_fb_sharing_mode_info,
	.set_xgmi_fb_sharing_mode = mi300_set_xgmi_fb_sharing_mode,
	.set_xgmi_fb_sharing_mode_ex = mi300_set_xgmi_fb_sharing_mode_ex,
	.get_shutdown_temperature = mi300_get_shutdown_temperature,
	.get_gpu_cache_info = mi300_get_gpu_cache_info,
	.get_max_pcie_link_generation = mi300_get_max_pcie_link_generation,
	.get_pm_policy = mi300_gpumon_smu_get_pm_policy,
	.set_pm_policy_level = mi300_gpumon_smu_set_pm_policy_level,
	.get_gfx_config = mi300_get_gfx_config,
};

static int mi300_gpumon_sw_init(struct amdgv_adapter *adapt)
{
	adapt->gpumon.funcs = &mi300_gpumon_funcs;

	adapt->i2c_cmd_buffer = oss_malloc(I2C_CMD_BUFFER_SIZE);
	if (adapt->i2c_cmd_buffer == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			I2C_CMD_BUFFER_SIZE);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mi300_gpumon_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->gpumon.funcs = NULL;
	oss_free(adapt->i2c_cmd_buffer);
	adapt->i2c_cmd_buffer = NULL;

	return 0;
}

static int mi300_gpumon_hw_init(struct amdgv_adapter *adapt)
{
	mi300_get_vbios_cache(adapt);
	return 0;
}

static int mi300_gpumon_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func mi300_gpumon_func = {
	.name = "mi300_gpumon_func",
	.sw_init = mi300_gpumon_sw_init,
	.sw_fini = mi300_gpumon_sw_fini,
	.hw_init = mi300_gpumon_hw_init,
	.hw_fini = mi300_gpumon_hw_fini,
};
