/*
 * Copyright (c) 2021-2023 Advanced Micro Devices, Inc. All rights reserved.
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


#include <amdgv.h>
#include <amdgv_device.h>
#include <amdgv_gpumon.h>
#include <amdgv_sched_internal.h>
#include <amdgv_oss_wrapper.h>
#include <amdgv_powerplay.h>
#include <amdgv_api_internal.h>
#include <amdgv_psp_gfx_if.h>

#include "mi200_powerplay.h"
#include "mi200_gpumon.h"
#include "mi200_xgmi.h"
#include "mi200_nbio.h"

#define mmFUSE_DATA_1				(0x17401)
#define mmFUSE_DATA_2				(0x17402)

/* Add hardcoded information to this table. */
struct mi200_gpumon_attribute_table {
	enum amdgv_gpumon_card_form_factor card_form_factor;
};

static struct mi200_gpumon_attribute_table mi200_attribute_table = {
	.card_form_factor = AMDGV_GPUMON_CARD_FORM_FACTOR__PCIE,
};

/* temperature = val/100 */
static int mi200_get_asic_temperature(struct amdgv_adapter *adapt, int *val)
{
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0)
			*val = metrics.temp_edge;
		else
			*val = 0;
	}

	return ret;
}

/* vddcr = val/100000 */
static int mi200_get_vddc(struct amdgv_adapter *adapt, int *val)
{
	uint8_t vdd;
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->get_pp_metrics) {
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

static int mi200_get_sclk(struct amdgv_adapter *adapt, int *val)
{
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0)
			*val = metrics.clocks[AMDGV_PP_CLK_SOC].curr;
		else
			*val = 0;
	}

	return ret;

}

static int mi200_get_mem_activity(struct amdgv_adapter *adapt, int *val)
{
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0)
			*val = metrics.mem_usage;
		else
			*val = 0;
	}

	return ret;
}

static int mi200_get_gpu_power_usage(struct amdgv_adapter *adapt, int *val)
{
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0)
			*val = metrics.power;
		else
			*val = 0;
	}

	return ret;
}

static int mi200_get_gfx_activity(struct amdgv_adapter *adapt, int *val)
{
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0)
			*val = metrics.gfx_usage;
		else
			*val = 0;
	}

	return ret;
}

static int mi200_get_gecc(struct amdgv_adapter *adapt, uint32_t *enabled)
{
	*enabled = adapt->ecc.enabled;
	return 0;
}

static int mi200_get_ecc_info(struct amdgv_adapter *adapt,
			      int *correctable_error,
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

static int mi200_get_vbios_info(struct amdgv_adapter *adapt,
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

static int mi200_get_card_form_factor(struct amdgv_adapter *adapt,
		enum amdgv_gpumon_card_form_factor *card_form_factor)
{
	*card_form_factor = mi200_attribute_table.card_form_factor;

	return 0;
}

static int mi200_get_gpu_power_capacity(struct amdgv_adapter *adapt,
					 int *val)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->get_power_capacity) {
		ret = adapt->pp.pp_funcs->get_power_capacity(adapt, val);
		if (ret != 0)
			*val = 0;
	}

	return ret;
}

static int mi200_get_dpm_capacity(struct amdgv_adapter *adapt,
				   int *val)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->get_dpm_capacity) {
		ret = adapt->pp.pp_funcs->get_dpm_capacity(adapt, val);
		if (ret != 0)
			*val = 0;
	}

	return ret;
}

static int mi200_get_dpm_status(struct amdgv_adapter *adapt,
				   int *val)
{
	struct amdgv_gpumon_metrics metrics;
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->get_pp_metrics) {
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, &metrics);
		if (ret == 0)
			*val = metrics.dpm;
		else
			*val = 0;
	}

	return ret;
}

static int mi200_get_max_sclk(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;
	uint32_t clk = 0;

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->get_clock_limit) {
		ret = adapt->pp.pp_funcs->get_clock_limit(adapt,
					PP_CLOCK_TYPE__GFX,
					PP_CLOCK_LIMIT_TYPE__SOFT_MAX,
					&clk);
		if (ret == 0)
			*val = (int)clk;
	}

	return ret;
}

static int mi200_get_max_mclk(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_FAILURE;
	uint32_t clk = 0;

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->get_clock_limit) {
		ret = adapt->pp.pp_funcs->get_clock_limit(adapt,
					PP_CLOCK_TYPE__UCLK,
					PP_CLOCK_LIMIT_TYPE__SOFT_MAX,
					&clk);
		if (ret == 0)
			*val = (int)clk;
	}

	return ret;
}

static int mi200_get_pp_metrics(struct amdgv_adapter *adapt,
		struct amdgv_gpumon_metrics *metrics)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->get_pp_metrics)
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, metrics);

	return ret;
}


static int mi200_get_xgmi_fb_sharing_caps(struct amdgv_adapter *adapt,
					  union amdgv_gpumon_xgmi_fb_sharing_caps *caps)
{
	caps->xgmi_fb_sharing_cap_mask = 0x0;

	if (adapt->xgmi.phy_nodes_num >= 2) {
		caps->mode_1_cap = true;
		caps->mode_2_cap = true;
	}

	if (adapt->xgmi.phy_nodes_num >= 4) {
		caps->mode_4_cap = true;
	}
	return 0;
}

static int mi200_get_xgmi_fb_sharing_mode_info(struct amdgv_adapter *src_adapt,
					       struct amdgv_adapter *dest_adapt,
					       enum amdgv_gpumon_xgmi_fb_sharing_mode mode,
					       uint8_t *is_sharing_enabled)
{
	struct amdgv_hive_info *hive;
	enum amdgv_xgmi_fb_sharing_mode libgv_mode;

	libgv_mode = gpumon_to_xgmi_fb_sharing_mode(mode);
	if (libgv_mode > MI200_XGMI_MAX_SUPPORTED_MODE)
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;

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

static int mi200_set_xgmi_fb_sharing_mode(struct amdgv_adapter *adapt,
			enum amdgv_gpumon_xgmi_fb_sharing_mode mode)
{
	int ret = AMDGV_FAILURE;
	enum amdgv_xgmi_fb_sharing_mode libgv_mode;

	libgv_mode = gpumon_to_xgmi_fb_sharing_mode(mode);
	if (libgv_mode > MI200_XGMI_MAX_SUPPORTED_MODE)
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;

	ret = amdgv_xgmi_update_topology_with_fb_sharing_mode(adapt, libgv_mode);

	/* Must be done in passive IRQL. Add job to workqueue. */
	oss_schedule_work(adapt->dev, save_fb_sharing_mode, (void *)adapt);

	return ret;
}

static int mi200_set_xgmi_fb_sharing_mode_ex(struct amdgv_adapter *adapt,
			enum amdgv_gpumon_xgmi_fb_sharing_mode mode, uint32_t sharing_mask)
{
	int ret = AMDGV_FAILURE;
	enum amdgv_xgmi_fb_sharing_mode libgv_mode;

	libgv_mode = gpumon_to_xgmi_fb_sharing_mode(mode);
	if (libgv_mode > MI200_XGMI_MAX_SUPPORTED_MODE)
		return AMDGV_ERROR_GPUMON_NOT_SUPPORTED;

	adapt->xgmi.custom_mode_sharing_mask = 0;

	ret = amdgv_xgmi_update_topology_with_fb_sharing_mode(adapt, libgv_mode);

	/* Must be done in passive IRQL. Add job to workqueue. */
	oss_schedule_work(adapt->dev, save_fb_sharing_mode, (void *)adapt);

	return ret;
}

static int mi200_is_pm_enabled(struct amdgv_adapter *adapt, bool *pm_enabled)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->is_pm_enabled) {
		ret = adapt->pp.pp_funcs->is_pm_enabled(adapt, pm_enabled);
	}

	return ret;
}

// TODO: Need double confirm the correctness of these values
#define MI200_XGMI_LINK_WEIGHT 15
#define MI200_PCIE_LINK_WEIGHT 20

static int mi200_get_link_topology(struct amdgv_adapter *adapt,
				   struct amdgv_adapter *dest_adapt,
				   struct amdgv_gpumon_link_topology_info *topology_info)
{
	int i;
	struct amdgv_hive_info *hive;
	struct amdgv_xgmi_psp_topology_info *psp_topology_info = NULL;
	struct amdgv_xgmi_psp_node_info *psp_node_info = NULL;
	struct amdgv_adapter *cur;

	/* TODO: Certain CPUs may disable Peer2Peer. Need to verify whether device is
	 * accessible */
	topology_info->link_status = AMDGV_GPUMON_LINK_STATUS_ENABLED;

	/* N/A on MI200X */
	if (dest_adapt == adapt) {
		topology_info->num_hops = 0;
		topology_info->weight = 0;
		topology_info->is_fb_sharing_enabled = true;
		topology_info->link_type = AMDGV_GPUMON_LINK_TYPE_NOT_APPLICABLE;
		return 0;
	}

	topology_info->is_fb_sharing_enabled = false;

	hive = amdgv_get_xgmi_hive(adapt);

	if (hive && amdgv_xgmi_is_node_in_hive(hive, dest_adapt)) {

		topology_info->link_type = AMDGV_GPUMON_LINK_TYPE_XGMI3;

		/* on MI200, we need to switch to PF to call this which is quite bad because
		 * MI200 doesn't officially support WS, and MI200 does not have custom mode
		 * so we use cached data if VF0(mi200 is 1VF only) is active */
		psp_topology_info = &adapt->xgmi.topology_info;
		if (!is_active_vf(0)) {
			if (amdgv_psp_xgmi_get_topology_info(adapt, hive, &adapt->xgmi.topology_info)) {
				topology_info->link_status = AMDGV_GPUMON_LINK_STATUS_ERROR;
				return 0;
			}
		}

		amdgv_list_for_each_entry(cur, &hive->adapt_list, struct amdgv_adapter, xgmi.head) {
			amdgv_xgmi_reflect_topology_info(cur, hive, &cur->xgmi.topology_info);
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
		topology_info->weight = MI200_XGMI_LINK_WEIGHT * topology_info->num_hops;
	}

	/* If XGMI FB Sharing is disabled / N/A, fallback on PCIE connection (not POR for MI200) */
	if (!topology_info->is_fb_sharing_enabled) {
		if (oss_get_device_numa_node(adapt->dev) == oss_get_device_numa_node(dest_adapt->dev))
			topology_info->num_hops = 2;
		else
			topology_info->num_hops = 3;

		topology_info->link_type = AMDGV_GPUMON_LINK_TYPE_PCIE;
		topology_info->weight = MI200_PCIE_LINK_WEIGHT * topology_info->num_hops;
	}

	return 0;
}

static int mi200_get_metrics_ext(struct amdgv_adapter *adapt,
	struct amdgv_gpumon_metrics_ext *metrics_ext)
{
	int ret = AMDGV_FAILURE;

	if (adapt->pp.pp_funcs->get_metrics_ext) {
		ret = adapt->pp.pp_funcs->get_metrics_ext(adapt, metrics_ext);
	}

	return ret;
}

static int mi200_get_max_pcie_link_generation(struct amdgv_adapter *adapt,
			int *val)
{
	*val = 4;

	return 0;
}

static int mi200_get_vram_info(struct amdgv_adapter *adapt,
				struct amdgv_gpumon_vram_info *vram_info)
{
	vram_info->vram_size_mb = mi200_nbio_get_total_vram_size(adapt);
	vram_info->vram_type = adapt->vram_info.vram_type;
	/* Program requested to remove vendor info from MI200 amd-smi */
	vram_info->vram_vendor = AMDGV_GPUMON_VRAM_VENDOR__PLACEHOLDER0;
	vram_info->vram_bit_width = adapt->vram_info.vram_bit_width;

	return 0;
}

static int mi200_get_ras_eeprom_version(struct amdgv_adapter *adapt,
	uint32_t *ras_eeprom_version)
{
	*ras_eeprom_version = 0xffffffff;
	return 0;
}

static int mi200_get_gfx_config(struct amdgv_adapter *adapt,
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

static const struct amdgv_gpumon_funcs mi200_gpumon_funcs = {
	.get_asic_temperature = mi200_get_asic_temperature,
	.get_gpu_power_usage = mi200_get_gpu_power_usage,
	.get_gpu_power_capacity = mi200_get_gpu_power_capacity,
	.get_dpm_cap = mi200_get_dpm_capacity,
	.get_dpm_status = mi200_get_dpm_status,
	.get_vddc = mi200_get_vddc,
	.get_sclk = mi200_get_sclk,
	.get_max_sclk   = mi200_get_max_sclk,
	.get_max_mclk   = mi200_get_max_mclk,
	.get_gfx_activity = mi200_get_gfx_activity,
	.get_mem_activity = mi200_get_mem_activity,
	.get_gecc = mi200_get_gecc,
	.get_ecc_info     = mi200_get_ecc_info,
	.get_vbios_info = mi200_get_vbios_info,
	.get_card_form_factor = mi200_get_card_form_factor,
	.get_pp_metrics = mi200_get_pp_metrics,
	.get_metrics_ext = mi200_get_metrics_ext,
	.get_link_topology = mi200_get_link_topology,
	.get_xgmi_fb_sharing_caps = mi200_get_xgmi_fb_sharing_caps,
	.get_xgmi_fb_sharing_mode_info = mi200_get_xgmi_fb_sharing_mode_info,
	.set_xgmi_fb_sharing_mode = mi200_set_xgmi_fb_sharing_mode,
	.set_xgmi_fb_sharing_mode_ex = mi200_set_xgmi_fb_sharing_mode_ex,
	.is_power_management_enabled = mi200_is_pm_enabled,
	.get_max_pcie_link_generation = mi200_get_max_pcie_link_generation,
	.get_vram_info = mi200_get_vram_info,
	.get_ras_eeprom_version = mi200_get_ras_eeprom_version,
	.get_gfx_config = mi200_get_gfx_config,
};

static int mi200_gpumon_sw_init(struct amdgv_adapter *adapt)
{
	adapt->gpumon.funcs = &mi200_gpumon_funcs;

	return 0;
}

static int mi200_gpumon_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->gpumon.funcs = NULL;

	return 0;
}

static int mi200_gpumon_hw_init(struct amdgv_adapter *adapt)
{
	struct amdgv_vbios_info *vbiosinfo = &adapt->vbios_cache;

	/* serial only needs to be init once */
	if (adapt->serial == 0) {
		if (adapt->gpumon.funcs->get_vbios_info) {
			vbiosinfo->serial = 0;
			adapt->gpumon.funcs->get_vbios_info(adapt,
				vbiosinfo);
			adapt->serial = vbiosinfo->serial;
		}
	}

	amdgv_vbios_cache_update(adapt);

	return 0;
}

static int mi200_gpumon_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func mi200_gpumon_func = {
	.name = "mi200_gpumon_func",
	.sw_init = mi200_gpumon_sw_init,
	.sw_fini = mi200_gpumon_sw_fini,
	.hw_init = mi200_gpumon_hw_init,
	.hw_fini = mi200_gpumon_hw_fini,
};
