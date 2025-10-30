/*
 * Copyright (C) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
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

#include "navi32_reg_inc.h"
#include "navi32_powerplay.h"
#include "navi32_nbio.h"

#define FUSE_DATA_248 (0x174F8)
#define FUSE_DATA_249 (0x174F9)
#define FUSE_DATA_250 (0x174FA)

static const uint32_t this_block = AMDGV_MANAGEMENT_BLOCK;

/* temperature = val/100 */
static int navi32_get_asic_temperature(struct amdgv_adapter *adapt, int *val)
{
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0)
			*val = metrics.temp_edge;
		else
			*val = 0;
	}

	return ret;
}

static int navi32_get_pp_metrics(struct amdgv_adapter *adapt,
				struct amdgv_gpumon_metrics *metrics)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_pp_metrics)
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, metrics);

	return ret;
}

/* vddcr = val/100000 */
static int navi32_get_vddc(struct amdgv_adapter *adapt, int *val)
{
	uint8_t vdd;
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0) {
			vdd = metrics.volt_gfx;
			*val = 155000 - vdd * 625;
		} else {
			*val = 0;
		}
	}

	return ret;
}

static int navi32_get_sclk(struct amdgv_adapter *adapt, int *val)
{
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0)
			*val = metrics.clocks[AMDGV_PP_CLK_SOC].curr;
		else
			*val = 0;
	}

	return ret;
}

/* internal funtion to get max clocks */
static int navi32_get_max_clk(struct amdgv_adapter *adapt, int *val, enum pp_clock_type clk_type)
{
	int ret = AMDGV_FAILURE;
	uint32_t clk = 0;

	if (adapt->pp.pp_funcs->get_clock_limit) {
		ret = adapt->pp.pp_funcs->get_clock_limit(adapt, clk_type,
							  PP_CLOCK_LIMIT_TYPE__SOFT_MAX, &clk);
	}
	*val = (int)clk;

	return ret;
}

/* internal funtion to get min clocks */
static int navi32_get_min_clk(struct amdgv_adapter *adapt, int *val, enum pp_clock_type clk_type)
{
	int ret = AMDGV_FAILURE;
	uint32_t clk = 0;

	if (adapt->pp.pp_funcs->get_clock_limit) {
		ret = adapt->pp.pp_funcs->get_clock_limit(adapt, clk_type,
							  PP_CLOCK_LIMIT_TYPE__SOFT_MIN, &clk);
	}
	*val = (int)clk;

	return ret;
}

static int navi32_get_max_sclk(struct amdgv_adapter *adapt, int *val)
{
	return navi32_get_max_clk(adapt, val, PP_CLOCK_TYPE__GFX);
}

static int navi32_get_max_mclk(struct amdgv_adapter *adapt, int *val)
{
	return navi32_get_max_clk(adapt, val, PP_CLOCK_TYPE__UCLK);
}

static int navi32_get_max_dclk0(struct amdgv_adapter *adapt, int *val)
{
	return navi32_get_max_clk(adapt, val, PP_CLOCK_TYPE__DCLK);
}

static int navi32_get_max_dclk1(struct amdgv_adapter *adapt, int *val)
{
	return navi32_get_max_clk(adapt, val, PP_CLOCK_TYPE__DCLK_1);
}

static int navi32_get_max_vclk0(struct amdgv_adapter *adapt, int *val)
{
	return navi32_get_max_clk(adapt, val, PP_CLOCK_TYPE__VCLK);
}

static int navi32_get_max_vclk1(struct amdgv_adapter *adapt, int *val)
{
	return navi32_get_max_clk(adapt, val, PP_CLOCK_TYPE__VCLK_1);
}

static int navi32_get_min_sclk(struct amdgv_adapter *adapt, int *val)
{
	return navi32_get_min_clk(adapt, val, PP_CLOCK_TYPE__GFX);
}

static int navi32_get_min_mclk(struct amdgv_adapter *adapt, int *val)
{
	return navi32_get_min_clk(adapt, val, PP_CLOCK_TYPE__UCLK);
}

static int navi32_get_min_vclk0(struct amdgv_adapter *adapt, int *val)
{
	return navi32_get_min_clk(adapt, val, PP_CLOCK_TYPE__VCLK);
}

static int navi32_get_min_vclk1(struct amdgv_adapter *adapt, int *val)
{
	return navi32_get_min_clk(adapt, val, PP_CLOCK_TYPE__VCLK_1);
}

static int navi32_get_min_dclk0(struct amdgv_adapter *adapt, int *val)
{
	return navi32_get_min_clk(adapt, val, PP_CLOCK_TYPE__DCLK);
}

static int navi32_get_min_dclk1(struct amdgv_adapter *adapt, int *val)
{
	return navi32_get_min_clk(adapt, val, PP_CLOCK_TYPE__DCLK_1);
}

static int navi32_get_mem_activity(struct amdgv_adapter *adapt, int *val)
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

static int navi32_get_gpu_power_usage(struct amdgv_adapter *adapt, int *val)
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

static int navi32_get_gfx_activity(struct amdgv_adapter *adapt, int *val)
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

/*
 * TODO: the two ecc functions are irrelevant to specific ASIC, and can be
 * moved to amdgv_gpumon.c
 */
static int navi32_gpumon_get_gecc(struct amdgv_adapter *adapt, uint32_t *enabled)
{
	*enabled = adapt->ecc.enabled;
	return 0;
}

static int navi32_gpumon_get_ecc_info(struct amdgv_adapter *adapt, int *correctable_error,
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

static int navi32_gpumon_clean_correctable_error_count(struct amdgv_adapter *adapt,
				     int *correctable_error)
{
	/* Query error info before clean erron count */
	if (adapt->ecc.get_correctable_error_count)
		*correctable_error = adapt->ecc.get_correctable_error_count(adapt, AMDGV_PF_IDX);
	else
		*correctable_error = 0;

	/* Clean correctable error count since driver is loaded done */
	if (adapt->ecc.correctable_error_num) {
		adapt->ecc.correctable_error_num = 0;
		*correctable_error = 0;
	}

	return 0;
}

static int navi32_get_vbios_info(struct amdgv_adapter *adapt,
				struct amdgv_vbios_info *vbiosinfo)
{
	uint64_t serial_low, serial_mid, serial_high;
	uint32_t pci_data;

	serial_low = RREG32(FUSE_DATA_248);
	serial_mid = RREG32(FUSE_DATA_249);
	serial_high = RREG32(FUSE_DATA_250);

	serial_low >>= 22;
	serial_mid <<= 10;
	serial_high <<= 42;

	vbiosinfo->serial = serial_low | serial_mid | serial_high;

	oss_pci_read_config_dword(adapt->dev, 0, &pci_data);
	vbiosinfo->dev_id = pci_data >> 16;
	pci_data = 0;
	oss_pci_read_config_dword(adapt->dev, 8, &pci_data);
	vbiosinfo->rev_id = pci_data & 0x000000FF;

	return 0;
}

static int navi32_gpumon_get_vbios_cache(struct amdgv_adapter *adapt)
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

static int navi32_get_gpu_power_capacity(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_power_capacity) {
		ret = adapt->pp.pp_funcs->get_power_capacity(adapt, val);
		if (ret != 0)
			*val = 0;
	}

	return ret;
}

static int navi32_get_dpm_capacity(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_dpm_capacity) {
		ret = adapt->pp.pp_funcs->get_dpm_capacity(adapt, val);
		if (ret != 0)
			*val = 0;
	}

	return ret;
}

static int navi32_is_pm_enabled(struct amdgv_adapter *adapt, bool *pm_enabled)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->is_pm_enabled) {
		ret = adapt->pp.pp_funcs->is_pm_enabled(adapt, pm_enabled);
	}

	return ret;
}

static int navi32_get_vram_info(struct amdgv_adapter *adapt,
				struct amdgv_gpumon_vram_info *vram_info)
{
	vram_info->vram_size_mb = navi32_nbio_get_total_vram_size(adapt);
	vram_info->vram_type = vram_type_to_gpumon_vram_type(adapt->vram_info.vram_type);
	vram_info->vram_vendor = vram_vendor_to_gpumon_vram_vendor(adapt->vram_info.vram_vendor);
	vram_info->vram_bit_width = adapt->vram_info.vram_bit_width;

	return 0;
}

static int navi32_get_gfx_config(struct amdgv_adapter *adapt,
	struct amdgv_gpumon_gfx_config *config)
{
	config->ip.hw_id = GC_HWIP;
	config->ip.full_ver = adapt->ip_versions[GC_HWIP][GET_INST(GC, 0)];

	config->max_shader_engines = adapt->config.gfx.max_shader_engines;
	config->max_cu_per_sh = adapt->config.gfx.max_cu_per_sh;
	config->max_sh_per_se = adapt->config.gfx.max_sh_per_se;
	config->max_waves_per_simd = adapt->config.gfx.max_waves_per_simd;
	config->wave_size = adapt->config.gfx.wave_size;
	config->active_cu_count = adapt->config.gfx.active_cu_count;

	return 0;
}

static int navi32_get_ras_eeprom_version(struct amdgv_adapter *adapt,
			uint32_t *ras_eeprom_version)
{
	*ras_eeprom_version = 0xffffffff;
	return 0;
}

static int navi32_get_ecc_correction_schema(struct amdgv_adapter *adapt,
	uint32_t *ecc_correction_schema)
{
	*ecc_correction_schema = 0xffffffff;
	return 0;
}

static const struct amdgv_gpumon_funcs navi32_gpumon_funcs = {
	.get_asic_temperature = navi32_get_asic_temperature,
	.get_gpu_power_usage = navi32_get_gpu_power_usage,
	.get_gpu_power_capacity = navi32_get_gpu_power_capacity,
	.get_vddc = navi32_get_vddc,

	.get_dpm_cap = navi32_get_dpm_capacity,

	.get_sclk = navi32_get_sclk,
	.get_gfx_activity = navi32_get_gfx_activity,
	.get_mem_activity = navi32_get_mem_activity,
	.get_gecc = navi32_gpumon_get_gecc,
	.get_ecc_info = navi32_gpumon_get_ecc_info,
	.clean_correctable_error_count = navi32_gpumon_clean_correctable_error_count,
	.ras_report = navi32_powerplay_ras_report,

	.get_vbios_info = navi32_get_vbios_info,
	.get_vbios_cache = navi32_gpumon_get_vbios_cache,
	.get_pp_metrics = navi32_get_pp_metrics,
	.get_max_sclk = navi32_get_max_sclk,
	.get_max_mclk = navi32_get_max_mclk,
	.get_max_dclk0 = navi32_get_max_dclk0,
	.get_max_dclk1 = navi32_get_max_dclk1,
	.get_max_vclk0 = navi32_get_max_vclk0,
	.get_max_vclk1 = navi32_get_max_vclk1,
	.get_min_sclk   = navi32_get_min_sclk,
	.get_min_mclk   = navi32_get_min_mclk,
	.get_min_vclk0  = navi32_get_min_vclk0,
	.get_min_vclk1  = navi32_get_min_vclk1,
	.get_min_dclk0  = navi32_get_min_dclk0,
	.get_min_dclk1  = navi32_get_min_dclk1,
	.get_vram_info  =  navi32_get_vram_info,
	.is_power_management_enabled = navi32_is_pm_enabled,
	.get_gfx_config = navi32_get_gfx_config,
	.get_ras_eeprom_version = navi32_get_ras_eeprom_version,
	.get_ecc_correction_schema = navi32_get_ecc_correction_schema,
};

static int navi32_gpumon_sw_init(struct amdgv_adapter *adapt)
{
	adapt->gpumon.funcs = &navi32_gpumon_funcs;

	adapt->i2c_cmd_buffer = oss_malloc(64);
	if (adapt->i2c_cmd_buffer == OSS_INVALID_HANDLE) {
		amdgv_put_error(AMDGV_PF_IDX, AMDGV_ERROR_DRIVER_ALLOC_SYSTEM_MEM_FAIL, 64);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int navi32_gpumon_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->gpumon.funcs = NULL;
	oss_free(adapt->i2c_cmd_buffer);
	adapt->i2c_cmd_buffer = NULL;

	return 0;
}

static int navi32_gpumon_hw_init(struct amdgv_adapter *adapt)
{
	navi32_gpumon_get_vbios_cache(adapt);

	/* cache product information */
	adapt->product_info.valid = true;
	if (adapt->pp.pp_funcs->get_fru_product_info)
		if (adapt->pp.pp_funcs->get_fru_product_info(adapt))
			AMDGV_WARN("Failed to get fru product info\n");

	return 0;
}

static int navi32_gpumon_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func navi32_gpumon_func = {
	.name = "navi32_gpumon_func",
	.sw_init = navi32_gpumon_sw_init,
	.sw_fini = navi32_gpumon_sw_fini,
	.hw_init = navi32_gpumon_hw_init,
	.hw_fini = navi32_gpumon_hw_fini,
};
