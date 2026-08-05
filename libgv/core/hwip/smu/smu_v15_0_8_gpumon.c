/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv.h>
#include <amdgv_device.h>
#include <amdgv_gpumon.h>
#include <amdgv_sched_internal.h>
#include <amdgv_oss_wrapper.h>
#include <amdgv_powerplay.h>
#include <amdgv_ip_discovery.h>
#include <amdgv_xgmi.h>
#include <amdgv_ual.h>
#include <amdgv_psp_gfx_if.h>
#include <amdgv_api_internal.h>

#include "hwip/psp/psp_v15_0_8.h"

static const uint32_t this_block = AMDGV_MANAGEMENT_BLOCK;

#define I2C_CMD_BUFFER_SIZE 80

/* Add hardcoded information to this table. */
// struct smu_v15_0_8_gpumon_attribute_table {
// 	enum amdgv_gpumon_card_form_factor card_form_factor;
// };

// static struct smu_v15_0_8_gpumon_attribute_table smu_v15_0_8_attribute_table = {
// 	.card_form_factor = AMDGV_GPUMON_CARD_FORM_FACTOR__OAM,
// };

static int smu_v15_0_8_get_ecc_info(struct amdgv_adapter *adapt, int *correctable_error,
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

static int smu_v15_0_8_get_vbios_cache(struct amdgv_adapter *adapt)
{
	return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
}

static int smu_v15_0_8_get_vbios_info(struct amdgv_adapter *adapt,
				struct amdgv_vbios_info *vbiosinfo)
{
	return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
}

static int smu_v15_0_8_get_gpu_power_capacity(struct amdgv_adapter *adapt, int *val,
					      enum amdgv_gpumon_type ppt_type)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	if (adapt->pp.pp_funcs->get_power_capacity) {
		ret = adapt->pp.pp_funcs->get_power_capacity(adapt, val, ppt_type);
		if (ret != 0)
			*val = 0;
	}

	return ret;
}

static int smu_v15_0_8_set_gpu_power_capacity(struct amdgv_adapter *adapt, int val)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	if (adapt->pp.pp_funcs->set_power_capacity)
		ret = adapt->pp.pp_funcs->set_power_capacity(adapt, val);

	return ret;
}

static int smu_v15_0_8_get_max_sclk(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;
	uint32_t clk = 0;

	if (adapt->pp.pp_funcs->get_clock_limit) {
		ret = adapt->pp.pp_funcs->get_clock_limit(adapt, PP_CLOCK_TYPE__GFX,
							  PP_CLOCK_LIMIT_TYPE__SOFT_MAX, &clk);
		if (ret == 0)
			*val = (int)clk;
	}

	return ret;
}


static int smu_v15_0_8_get_max_vclk0(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;
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

static int smu_v15_0_8_get_max_vclk1(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;
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

static int smu_v15_0_8_get_max_dclk0(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;
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

static int smu_v15_0_8_get_max_dclk1(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;
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

static int smu_v15_0_8_get_min_sclk(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;
	uint32_t clk = 0;

	if (adapt->pp.pp_funcs->get_clock_limit) {
		ret = adapt->pp.pp_funcs->get_clock_limit(adapt, PP_CLOCK_TYPE__GFX,
							  PP_CLOCK_LIMIT_TYPE__SOFT_MIN, &clk);
		if (ret == 0)
			*val = (int)clk;
	}

	return ret;
}

// static int smu_v15_0_8_get_min_mclk(struct amdgv_adapter *adapt, int *val)

static int smu_v15_0_8_get_min_vclk0(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;
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

static int smu_v15_0_8_get_min_vclk1(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;
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

static int smu_v15_0_8_get_min_dclk0(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;
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

static int smu_v15_0_8_get_min_dclk1(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;
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

static int smu_v15_0_8_get_pp_metrics(struct amdgv_adapter *adapt,
				      struct amdgv_gpumon_metrics *metrics)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	if (adapt->pp.pp_funcs->get_pp_metrics)
		ret = adapt->pp.pp_funcs->get_pp_metrics(adapt, metrics);

	return ret;
}

static int smu_v15_0_8_get_vram_info(struct amdgv_adapter *adapt,
				     struct amdgv_gpumon_vram_info *vram_info)
{
	// vram_info->vram_size_mb = smu_v15_0_8_nbio_get_total_vram_size(adapt);
	// vram_info->vram_type = vram_type_to_gpumon_vram_type(adapt->vram_info.vram_type);
	// vram_info->vram_vendor = vram_vendor_to_gpumon_vram_vendor(adapt->vram_info.vram_vendor);
	// vram_info->vram_bit_width = adapt->vram_info.vram_bit_width;
	return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
}

static int smu_v15_0_8_is_clk_locked(struct amdgv_adapter *adapt,
				     enum AMDGV_PP_CLK_DOMAIN clk_domain, uint8_t *clk_locked)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	if (adapt->pp.pp_funcs->is_clock_locked)
		ret = adapt->pp.pp_funcs->is_clock_locked(adapt, clk_domain, clk_locked);

	return ret;
}

static inline const char *smu_v15_0_8_get_memory_partition_mode_desc(
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

static const char *smu_v15_0_8_get_accelerator_partition_mode_desc(struct amdgv_adapter *adapt,
				enum amdgv_accelerator_partition_mode accelerator_partition_mode)
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

static int smu_v15_0_8_get_memory_partition_config(struct amdgv_adapter *adapt,
						   union amdgv_gpumon_memory_partition_config
						   *memory_partition_config)
{
	if (adapt == NULL || memory_partition_config == NULL)
		return AMDGV_LOG_GPUMON_INVALID_OPTION;

	memory_partition_config->mp_cap_mask = 0;
	memory_partition_config->mp_caps.nps1_cap = 1;

	return 0;
}

static int smu_v15_0_8_set_memory_partition_mode(struct amdgv_adapter *adapt,
						 enum amdgv_memory_partition_mode
						 memory_partition_mode)
{
	enum psp_status psp_ret = PSP_STATUS__SUCCESS;
	enum amdgv_accelerator_partition_mode default_accel_mode;

	default_accel_mode = amdgv_nbio_get_default_accel_partition_mode(adapt,
						memory_partition_mode);
	if (!default_accel_mode)
		return AMDGV_LOG_GPUMON_INVALID_MODE;

	if (!amdgv_nbio_is_partition_mode_supported(adapt, memory_partition_mode,
						    default_accel_mode)) {
		AMDGV_ERROR("requested NPS%u mode is not supported\n", memory_partition_mode);
		return AMDGV_LOG_GPUMON_INVALID_MODE;
	}

	psp_ret = psp_v15_0_8_set_memory_partition_mode(adapt, memory_partition_mode);
	if (psp_ret != PSP_STATUS__SUCCESS)
		return AMDGV_FAILURE;

	psp_ret = psp_v15_0_8_set_accelerator_partition_mode(adapt, default_accel_mode);
	if (psp_ret != PSP_STATUS__SUCCESS)
		return AMDGV_FAILURE;

	return 0;
}

static int smu_v15_0_8_set_spatial_partition_num(struct amdgv_adapter *adapt,
						 uint32_t spatial_partition_num)
{
	return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
}

/* Find a resource profile matching (type, per-partition amount, share count) or
 * append a new one, returning its index.
 */
static uint32_t smu_v15_0_8_get_resource_profile_index(
	struct amdgv_gpumon_accelerator_partition_profile_config *cfg,
	enum amdgv_gpumon_accelerator_partition_resource_type type,
	uint32_t partition_resource, uint32_t share)
{
	uint32_t i = cfg->number_of_resource_profiles;
	uint32_t j;

	for (j = 0; j < i; j++) {
		if (cfg->resource_profiles[j].resource_type == type &&
		    cfg->resource_profiles[j].partition_resource == partition_resource &&
		    cfg->resource_profiles[j].num_partitions_share_resource == share)
			return j;
	}

	cfg->resource_profiles[i].resource_index = i;
	cfg->resource_profiles[i].resource_type = type;
	cfg->resource_profiles[i].partition_resource = partition_resource;
	cfg->resource_profiles[i].num_partitions_share_resource = share;
	cfg->number_of_resource_profiles = i + 1;

	return i;
}

/* Which VF counts each partition mode is offered for. */
static uint32_t smu_v15_0_8_mode_support_vf_num(enum spatial_partition_mode mode)
{
	switch (mode) {
	case SPATIAL_PARTITION_MODE__SPX:
		return (1 << 1);
	case SPATIAL_PARTITION_MODE__DPX:
		return (1 << 2);
	case SPATIAL_PARTITION_MODE__QPX:
		return (1 << 4);
	case SPATIAL_PARTITION_MODE__CPX:
		return (1 << 1) | (1 << 2) | (1 << 4) | (1 << 6) | (1 << 8);
	default:
		return 0;
	}
}

static enum amdgv_gpumon_acccelerator_partition_type
smu_v15_0_8_mode_to_profile_type(enum spatial_partition_mode mode)
{
	switch (mode) {
	case SPATIAL_PARTITION_MODE__SPX:
		return AMDGV_GPUMON_ACCELERATOR_PARTITION_SPX;
	case SPATIAL_PARTITION_MODE__DPX:
		return AMDGV_GPUMON_ACCELERATOR_PARTITION_DPX;
	case SPATIAL_PARTITION_MODE__QPX:
		return AMDGV_GPUMON_ACCELERATOR_PARTITION_QPX;
	case SPATIAL_PARTITION_MODE__CPX:
		return AMDGV_GPUMON_ACCELERATOR_PARTITION_CPX;
	default:
		return AMDGV_GPUMON_ACCELERATOR_PARTITION_INVALID;
	}
}

/* Convert one spatial partition layout into a gpumon profile. Each partition
 * reports its XCC count and its decoder count; a decoder's share is how many
 * partitions map onto that same VCN, so a partition holding a shared VCN
 * reports the exact sharing.
 */
static void smu_v15_0_8_add_profile(
	struct amdgv_gpumon_accelerator_partition_profile_config *cfg,
	const struct amdgv_spatial_partition_layout *layout)
{
	struct amdgv_gpumon_acccelerator_partition_profile *p =
		&cfg->profiles[cfg->number_of_profiles];
	uint32_t parts_per_vcn[AMDGV_MCP_MAX_SPATIAL_PARTITIONS] = { 0 };
	uint32_t i, v;

	for (i = 0; i < layout->num_partitions; i++)
		for_each_id (v, layout->part[i].vcn_mask)
			parts_per_vcn[v]++;

	p->profile_index = cfg->number_of_profiles;
	p->profile_type = smu_v15_0_8_mode_to_profile_type(layout->mode);
	p->memory_caps.mp_caps.nps1_cap = 1;
	if (layout->mode != SPATIAL_PARTITION_MODE__SPX)
		p->memory_caps.mp_caps.nps2_cap = 1;
	p->num_partitions = layout->num_partitions;
	p->num_resources = 2;
	p->support_vf_num = smu_v15_0_8_mode_support_vf_num(layout->mode);

	for (i = 0; i < layout->num_partitions; i++) {
		uint32_t num_xcc = 0, num_vcn = 0, share = 1;

		for_each_id (v, layout->part[i].xcc_mask)
			num_xcc++;
		for_each_id (v, layout->part[i].vcn_mask) {
			num_vcn++;
			if (parts_per_vcn[v] > share)
				share = parts_per_vcn[v];
		}

		p->partition_id[i] = i;
		p->resources[i][0] = smu_v15_0_8_get_resource_profile_index(cfg,
			AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_XCC, num_xcc, 1);
		p->resources[i][1] = smu_v15_0_8_get_resource_profile_index(cfg,
			AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_DECODER, num_vcn, share);
	}

	cfg->number_of_profiles++;
}

/* Format the adapter's prebuilt spatial partition layouts as gpumon profiles. */
static void smu_v15_0_8_build_accelerator_partition_profile_config(struct amdgv_adapter *adapt,
	struct amdgv_gpumon_accelerator_partition_profile_config *cfg)
{
	uint32_t i;

	oss_memset(cfg, 0, sizeof(*cfg));

	for (i = 0; i < adapt->mcp.num_partition_layouts; i++)
		smu_v15_0_8_add_profile(cfg, &adapt->mcp.partition_layouts[i]);
}

static struct amdgv_gpumon_accelerator_partition_profile_config
	smu_v15_0_8_accelerator_partition_profile_configs_valid;

/* Build the config for the current num_vf: the full set of profiles, then drop
 * the ones the current num_vf cannot use, compacting the survivors in place.
 */
static struct amdgv_gpumon_accelerator_partition_profile_config *
smu_v15_0_8_get_accelerator_partition_profile_asic_config(struct amdgv_adapter *adapt)
{
	struct amdgv_gpumon_accelerator_partition_profile_config *cfg =
		&smu_v15_0_8_accelerator_partition_profile_configs_valid;
	uint32_t valid_profiles = 0;
	uint32_t i;

	if (adapt->mcp.gfx.num_xcc == 0)
		return NULL;

	smu_v15_0_8_build_accelerator_partition_profile_config(adapt, cfg);

	for (i = 0; i < cfg->number_of_profiles; i++) {
		if (!(cfg->profiles[i].support_vf_num & (1 << adapt->num_vf)))
			continue;
		if (valid_profiles != i)
			cfg->profiles[valid_profiles] = cfg->profiles[i];
		cfg->profiles[valid_profiles].profile_index = valid_profiles;
		valid_profiles++;
	}
	cfg->number_of_profiles = valid_profiles;

	return cfg;
}

static int smu_v15_0_8_get_accelerator_partition_profile_config_global(struct amdgv_adapter *adapt,
	struct amdgv_gpumon_accelerator_partition_profile_config *profile_configs)
{
	if (adapt == NULL || profile_configs == NULL)
		return AMDGV_LOG_GPUMON_INVALID_OPTION;

	if (adapt->mcp.gfx.num_xcc == 0)
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	smu_v15_0_8_build_accelerator_partition_profile_config(adapt, profile_configs);

	return 0;
}

static int smu_v15_0_8_get_accelerator_partition_profile_config(struct amdgv_adapter *adapt,
	struct amdgv_gpumon_accelerator_partition_profile_config *profile_configs)
{
	struct amdgv_gpumon_accelerator_partition_profile_config *profile_asic_configs;

	if (adapt == NULL || profile_configs == NULL)
		return AMDGV_LOG_GPUMON_INVALID_OPTION;

	profile_asic_configs = smu_v15_0_8_get_accelerator_partition_profile_asic_config(adapt);
	if (profile_asic_configs == NULL)
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	oss_memcpy(profile_configs, profile_asic_configs,
		   sizeof(struct amdgv_gpumon_accelerator_partition_profile_config));

	return 0;
}

static uint32_t smu_v15_0_8_get_accelerator_partition_mode(struct amdgv_adapter *adapt,
					       uint32_t profile_index)
{
	struct amdgv_gpumon_accelerator_partition_profile_config *cfg;
	uint32_t i;

	cfg = smu_v15_0_8_get_accelerator_partition_profile_asic_config(adapt);
	if (cfg == NULL)
		return 0;

	for (i = 0; i < cfg->number_of_profiles; i++)
		if (cfg->profiles[i].profile_index == profile_index)
			return amdgv_gpumon_partition_type_to_mode(cfg->profiles[i].profile_type);

	return 0;
}

static int smu_v15_0_8_set_accelerator_partition_profile(struct amdgv_adapter *adapt,
							 uint32_t profile_index)
{
	int ret;
	enum psp_status psp_ret = PSP_STATUS__SUCCESS;
	uint32_t req_accelerator_partition_mode;
	enum amdgv_memory_partition_mode curr_memory_partition_mode;

	req_accelerator_partition_mode =
		smu_v15_0_8_get_accelerator_partition_mode(adapt, profile_index);
	if (!req_accelerator_partition_mode) {
		AMDGV_ERROR(
			"requested accelerator_partition_profile_index=%u is not supported\n",
			profile_index);
		return AMDGV_FAILURE;
	}

	ret = amdgv_nbio_get_nps_mode(adapt, &curr_memory_partition_mode);
	if (ret || (curr_memory_partition_mode != adapt->mcp.memory_partition_mode)) {
		AMDGV_ERROR("failed to get current memory partition mode or memory partition mode mismatch\n");
		return AMDGV_FAILURE;
	}

	/* Requested accelerator partitions should not be less than memory partitions
	 * for example, if the requested compute mode is DPX, but current NPS mode is NPS4,
	 * host driver should fail the request with indication that DPX is not supported with NPS4. */
	if (amdgv_nbio_is_partition_mode_supported(adapt,
			curr_memory_partition_mode,
			req_accelerator_partition_mode) == false) {
		AMDGV_ERROR(
			"requested accelerator_partition_mode=%s is not supported with %s mode and %uVF\n",
			smu_v15_0_8_get_accelerator_partition_mode_desc(
				adapt, req_accelerator_partition_mode),
			smu_v15_0_8_get_memory_partition_mode_desc(
				curr_memory_partition_mode),
			adapt->num_vf);
		return AMDGV_LOG_GPUMON_INVALID_MODE;
	}

	/* Request PSP to switch compute partition mode */
	psp_ret = psp_v15_0_8_set_accelerator_partition_mode(
		adapt, req_accelerator_partition_mode);
	if (psp_ret != PSP_STATUS__SUCCESS) {
		return AMDGV_FAILURE;
	}

	/* Re-init partition mapping for all metrics */
	if (adapt->pp.pp_funcs && adapt->pp.pp_funcs->init_drv_metrics_ext)
		adapt->pp.pp_funcs->init_drv_metrics_ext(adapt);

	return 0;
}

static int smu_v15_0_8_get_accelerator_partition_profile(struct amdgv_adapter *adapt,
					struct amdgv_gpumon_acccelerator_partition_profile *profile)
{
	enum amdgv_accelerator_partition_mode accelerator_partition_mode;
	struct amdgv_gpumon_accelerator_partition_profile_config
		*profile_asic_configs;
	uint32_t i;

	if (adapt == NULL || profile == NULL)
		return AMDGV_LOG_GPUMON_INVALID_OPTION;

	profile_asic_configs = smu_v15_0_8_get_accelerator_partition_profile_asic_config(adapt);
	if (profile_asic_configs == NULL)
		return AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	accelerator_partition_mode = amdgv_nbio_get_accel_partition_mode(adapt);
	if (!accelerator_partition_mode) {
		AMDGV_ERROR("failed to get current accelerator_partition_mode\n");
		return AMDGV_FAILURE;
	}

	for (i = 0; i < profile_asic_configs->number_of_profiles; i++) {
		if (amdgv_gpumon_partition_type_to_mode(profile_asic_configs->profiles[i].profile_type) ==
		    accelerator_partition_mode) {
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

static int smu_v15_0_8_reset_spatial_partition_num(struct amdgv_adapter *adapt)
{
	return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
}

static int smu_v15_0_8_get_spatial_partition_caps(struct amdgv_adapter *adapt,
						  struct amdgv_gpumon_spatial_partition_caps
						  *spatial_partition_caps)
{
	if (adapt == NULL || spatial_partition_caps == NULL)
		return AMDGV_LOG_GPUMON_INVALID_OPTION;

	spatial_partition_caps->num_xcc = adapt->mcp.gfx.num_xcc;
	spatial_partition_caps->num_sdma = adapt->sdma.num_instances;
	spatial_partition_caps->num_vcn = adapt->config.mm.count[AMDGV_VCN_ENGINE];
	spatial_partition_caps->num_jpeg = 2 * adapt->config.mm.count[AMDGV_VCN_ENGINE];

	return 0;
}

static int smu_v15_0_8_get_memory_partition_mode(struct amdgv_adapter *adapt,
						 struct amdgv_gpumon_memory_partition_info
						 *memory_partition_info)
{
	uint32_t i;
	int ret;

	if (adapt == NULL || memory_partition_info == NULL) {
		return AMDGV_LOG_GPUMON_INVALID_OPTION;
	}

	ret = amdgv_nbio_get_nps_mode(adapt, &memory_partition_info->memory_partition_mode);
	if (ret)
		return ret;

	memory_partition_info->num_numa_ranges = adapt->mcp.numa_count;
	for (i = 0; i < memory_partition_info->num_numa_ranges; i++) {
		memory_partition_info->numa_range[i].memory_type =adapt->vram_info.vram_type;
		memory_partition_info->numa_range[i].start = adapt->mcp.numa_range[i].start;
		memory_partition_info->numa_range[i].end = adapt->mcp.numa_range[i].end;
	}

	return 0;
}

static int smu_v15_0_8_get_spatial_partition_num(struct amdgv_adapter *adapt,
						 uint32_t *spatial_partition_num)
{
	*spatial_partition_num = adapt->sched.num_spatial_partitions;

	return 0;
}

static int smu_v15_0_8_get_shutdown_temperature(struct amdgv_adapter *adapt, int *val)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	if (adapt->pp.pp_funcs->get_shutdown_temperature) {
		ret = adapt->pp.pp_funcs->get_shutdown_temperature(adapt, val);
	}

	return ret;
}

static int smu_v15_0_8_get_pcie_replay_count(struct amdgv_adapter *adapt, int *val)
{
//	*val = smu_v15_0_8_nbio_get_pcie_replay_count(adapt);

	return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
}

static int smu_v15_0_8_get_card_form_factor(struct amdgv_adapter *adapt,
					    enum amdgv_gpumon_card_form_factor *card_form_factor)
{
	//*card_form_factor = smu_v15_0_8_attribute_table.card_form_factor;

	return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
}

static int smu_v15_0_8_get_max_configurable_power_limit(struct amdgv_adapter *adapt,
							int *power_limit)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	if (adapt->pp.pp_funcs->get_max_configurable_power_limit)
		ret = adapt->pp.pp_funcs->get_max_configurable_power_limit(adapt, power_limit);

	return ret;
}

static int smu_v15_0_8_get_default_power_limit(struct amdgv_adapter *adapt, int *default_power)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	if (adapt->pp.pp_funcs->get_default_power_limit)
		ret = adapt->pp.pp_funcs->get_default_power_limit(adapt, default_power);

	return ret;
}

/* PMFW allows to set power to zero so the min limit is hardcoded */
static int smu_v15_0_8_get_min_power_limit(struct amdgv_adapter *adapt, int *val)
{
	*val = 0;

	return 0;
}

static int smu_v15_0_8_get_metrics_ext(struct amdgv_adapter *adapt,
				       struct amdgv_gpumon_metrics_ext *metrics_ext)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	if (adapt->pp.pp_funcs->get_metrics_ext)
		ret = adapt->pp.pp_funcs->get_metrics_ext(adapt, metrics_ext);

	return ret;
}

static int smu_v15_0_8_get_num_metrics_ext_entries(struct amdgv_adapter *adapt,
						   uint32_t *entries)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	if (adapt->pp.pp_funcs->get_num_metrics_ext_entries)
		ret = adapt->pp.pp_funcs->get_num_metrics_ext_entries(adapt, entries);

	return ret;
}

static int smu_v15_0_8_get_static_metrics_ext(struct amdgv_adapter *adapt,
					      struct amdgv_gpumon_metrics_ext *metrics_ext)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	if (adapt->pp.pp_funcs->get_static_metrics_ext)
		ret = adapt->pp.pp_funcs->get_static_metrics_ext(adapt, metrics_ext);

	return ret;
}

static int smu_v15_0_8_get_num_static_metrics_ext_entries(struct amdgv_adapter *adapt,
							  uint32_t *entries)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	if (adapt->pp.pp_funcs->get_num_static_metrics_ext_entries)
		ret = adapt->pp.pp_funcs->get_num_static_metrics_ext_entries(adapt, entries);

	return ret;
}

static int smu_v15_0_8_is_pm_enabled(struct amdgv_adapter *adapt, bool *pm_enabled)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	if (adapt->pp.pp_funcs->is_pm_enabled)
		ret = adapt->pp.pp_funcs->is_pm_enabled(adapt, pm_enabled);

	return ret;
}

static int smu_v15_0_8_gpumon_smu_get_pm_policy(struct amdgv_adapter *adapt,
						enum amdgv_pp_pm_policy p_type,
						struct amdgv_gpumon_smu_dpm_policy *policy)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;
	struct pp_smu_dpm_policy *policy_int;
	uint32_t i = 0;

	if (adapt->pp.pp_funcs->smu_get_pm_policy)
		ret = adapt->pp.pp_funcs->smu_get_pm_policy(adapt, p_type, &policy_int);

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

static int smu_v15_0_8_gpumon_smu_set_pm_policy_level(struct amdgv_adapter *adapt,
						      enum amdgv_pp_pm_policy p_type,
						      enum amdgv_pp_policy_soc_pstate level)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	if (adapt->pp.pp_funcs->smu_compare_and_set_pm_policy)
		ret = adapt->pp.pp_funcs->smu_compare_and_set_pm_policy(adapt, p_type, level);

	return ret;
}

static int smu_v15_0_8_get_npm_info(struct amdgv_adapter *adapt,
				    struct amdgv_gpumon_npm_info *npm_info)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	if (adapt->pp.pp_funcs->get_npm_info)
		ret = adapt->pp.pp_funcs->get_npm_info(adapt, npm_info);

	return ret;
}

static int smu_v15_0_8_get_link_metrics(struct amdgv_adapter *adapt,
					struct amdgv_gpumon_link_metrics *link_metrics)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	if (adapt->pp.pp_funcs->get_link_metrics)
		ret = adapt->pp.pp_funcs->get_link_metrics(adapt, link_metrics);

	return ret;
}

static int smu_v15_0_8_get_link_topology(struct amdgv_adapter *adapt,
					 struct amdgv_adapter *dest_adapt,
					 struct amdgv_gpumon_link_topology_info *topology_info)
{
	return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
}

static int smu_v15_0_8_get_xgmi_fb_sharing_caps(struct amdgv_adapter *adapt,
						union amdgv_gpumon_xgmi_fb_sharing_caps *caps)
{
	return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
}

static int smu_v15_0_8_get_xgmi_fb_sharing_mode_info(struct amdgv_adapter *src_adapt,
						     struct amdgv_adapter *dest_adapt,
						     enum amdgv_gpumon_xgmi_fb_sharing_mode mode,
						     uint8_t *is_sharing_enabled)
{
	return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
}

static int smu_v15_0_8_set_xgmi_fb_sharing_mode(struct amdgv_adapter *adapt,
						enum amdgv_gpumon_xgmi_fb_sharing_mode mode)
{
	return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
}

static int smu_v15_0_8_set_xgmi_fb_sharing_mode_ex(struct amdgv_adapter *adapt,
						   enum amdgv_gpumon_xgmi_fb_sharing_mode mode,
						   uint32_t sharing_mask)
{
	return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
}

static int smu_v15_0_8_get_gpu_cache_info(struct amdgv_adapter *adapt,
					  struct amdgv_gpumon_gpu_cache_info *gpu_cache_info)
{
	/* IP Discovery Not ready yet */
	return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
}

static int smu_v15_0_8_get_max_pcie_link_generation(struct amdgv_adapter *adapt,
						    int *val)
{
	return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
}

static int smu_v15_0_8_get_gfx_config(struct amdgv_adapter *adapt,
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

static int smu_v15_0_8_get_ecc_correction_schema(struct amdgv_adapter *adapt,
						 uint32_t *ecc_correction_schema)
{
	return AMDGV_LOG_GPUMON_NOT_SUPPORTED;
}

static int smu_v15_0_8_ual_get_interface_version(struct amdgv_adapter *adapt, uint32_t *version)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	ret = amdgv_ual_get_interface_version(adapt, version);

	return ret;
}

static int smu_v15_0_8_ual_get_config(struct amdgv_adapter *adapt,
					struct amdgv_gpumon_get_config_rsp_ual_v1 *config)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	ret = amdgv_ual_get_config(adapt, config);

	return ret;
}

static int smu_v15_0_8_ual_set_ppod_config(struct amdgv_adapter *adapt,
					struct amdgv_gpumon_set_ppod_config_req_ual_v1 *config)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	ret = amdgv_ual_set_ppod_config(adapt, config);

	return ret;
}

static int smu_v15_0_8_ual_set_vpod_config(struct amdgv_adapter *adapt,
					struct amdgv_gpumon_set_vpod_config_req_ual_v1 *config)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	ret = amdgv_ual_set_vpod_config(adapt, config);

	return ret;
}

static int smu_v15_0_8_ual_set_station_config(struct amdgv_adapter *adapt,
					struct amdgv_gpumon_set_station_config_req_ual_v1 *config)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	ret = amdgv_ual_set_station_config(adapt, config);

	return ret;
}

static int smu_v15_0_8_ual_pause(struct amdgv_adapter *adapt)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	ret = amdgv_ual_pause(adapt, false);

	return ret;
}

static int smu_v15_0_8_ual_resume(struct amdgv_adapter *adapt)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	ret = amdgv_ual_resume(adapt, false);

	return ret;
}

static int smu_v15_0_8_ual_trigger_mode2(struct amdgv_adapter *adapt)
{
	int ret = AMDGV_LOG_GPUMON_NOT_SUPPORTED;

	ret = amdgv_ual_trigger_mode2(adapt);

	return ret;
}

static const struct amdgv_gpumon_funcs smu_v15_0_8_gpumon_funcs = {
	.get_asic_temperature = NULL,
	.get_gpu_power_usage = NULL,
	.get_dpm_cap = NULL,
	.get_dpm_status = NULL,
	.get_sclk = NULL,
	.get_max_mclk = NULL,
	.get_min_mclk = NULL,
	.get_gfx_activity = NULL,
	.get_mem_activity = NULL,
	.get_gecc = NULL,
	.get_gpu_power_capacity = smu_v15_0_8_get_gpu_power_capacity,
	.set_gpu_power_capacity = smu_v15_0_8_set_gpu_power_capacity,
	.get_max_sclk = smu_v15_0_8_get_max_sclk,
	.get_max_vclk0 = smu_v15_0_8_get_max_vclk0,
	.get_max_vclk1 = smu_v15_0_8_get_max_vclk1,
	.get_max_dclk0 = smu_v15_0_8_get_max_dclk0,
	.get_max_dclk1 = smu_v15_0_8_get_max_dclk1,
	.get_min_sclk = smu_v15_0_8_get_min_sclk,
	.get_min_vclk0 = smu_v15_0_8_get_min_vclk0,
	.get_min_vclk1 = smu_v15_0_8_get_min_vclk1,
	.get_min_dclk0 = smu_v15_0_8_get_min_dclk0,
	.get_min_dclk1 = smu_v15_0_8_get_min_dclk1,
	.get_ecc_info = smu_v15_0_8_get_ecc_info,
	.get_pp_metrics = smu_v15_0_8_get_pp_metrics,
	.get_vbios_info = smu_v15_0_8_get_vbios_info,
	.get_vbios_cache = smu_v15_0_8_get_vbios_cache,
	.get_vram_info = smu_v15_0_8_get_vram_info,
	.is_clk_locked = smu_v15_0_8_is_clk_locked,
	.get_accelerator_partition_profile_config_global = smu_v15_0_8_get_accelerator_partition_profile_config_global,
	.get_accelerator_partition_profile_config = smu_v15_0_8_get_accelerator_partition_profile_config,
	.set_accelerator_partition_profile = smu_v15_0_8_set_accelerator_partition_profile,
	.get_accelerator_partition_profile = smu_v15_0_8_get_accelerator_partition_profile,
	.get_memory_partition_config = smu_v15_0_8_get_memory_partition_config,
	.get_spatial_partition_caps = smu_v15_0_8_get_spatial_partition_caps,
	.set_memory_partition_mode = smu_v15_0_8_set_memory_partition_mode,
	.set_spatial_partition_num = smu_v15_0_8_set_spatial_partition_num,
	.reset_spatial_partition_num = smu_v15_0_8_reset_spatial_partition_num,
	.get_memory_partition_mode = smu_v15_0_8_get_memory_partition_mode,
	.get_spatial_partition_num = smu_v15_0_8_get_spatial_partition_num,
	.get_pcie_replay_count = smu_v15_0_8_get_pcie_replay_count,
	.get_card_form_factor = smu_v15_0_8_get_card_form_factor,
	.get_max_configurable_power_limit = smu_v15_0_8_get_max_configurable_power_limit,
	.get_default_power_limit = smu_v15_0_8_get_default_power_limit,
	.get_min_power_limit = smu_v15_0_8_get_min_power_limit,
	.get_metrics_ext = smu_v15_0_8_get_metrics_ext,
	.get_num_metrics_ext_entries = smu_v15_0_8_get_num_metrics_ext_entries,
	.is_power_management_enabled = smu_v15_0_8_is_pm_enabled,
	.get_link_metrics = smu_v15_0_8_get_link_metrics,
	.get_link_topology = smu_v15_0_8_get_link_topology,
	.get_xgmi_fb_sharing_caps = smu_v15_0_8_get_xgmi_fb_sharing_caps,
	.get_xgmi_fb_sharing_mode_info = smu_v15_0_8_get_xgmi_fb_sharing_mode_info,
	.set_xgmi_fb_sharing_mode = smu_v15_0_8_set_xgmi_fb_sharing_mode,
	.set_xgmi_fb_sharing_mode_ex = smu_v15_0_8_set_xgmi_fb_sharing_mode_ex,
	.get_shutdown_temperature = smu_v15_0_8_get_shutdown_temperature,
	.get_gpu_cache_info = smu_v15_0_8_get_gpu_cache_info,
	.get_max_pcie_link_generation = smu_v15_0_8_get_max_pcie_link_generation,
	.get_pm_policy = smu_v15_0_8_gpumon_smu_get_pm_policy,
	.set_pm_policy_level = smu_v15_0_8_gpumon_smu_set_pm_policy_level,
	.get_gfx_config = smu_v15_0_8_get_gfx_config,
	.get_ecc_correction_schema = smu_v15_0_8_get_ecc_correction_schema,
	.get_static_metrics_ext = smu_v15_0_8_get_static_metrics_ext,
	.get_num_static_metrics_ext_entries = smu_v15_0_8_get_num_static_metrics_ext_entries,
	.get_npm_info = smu_v15_0_8_get_npm_info,
	.ual_get_interface_version = smu_v15_0_8_ual_get_interface_version,
	.ual_get_config = smu_v15_0_8_ual_get_config,
	.ual_set_ppod_config = smu_v15_0_8_ual_set_ppod_config,
	.ual_set_vpod_config = smu_v15_0_8_ual_set_vpod_config,
	.ual_set_station_config = smu_v15_0_8_ual_set_station_config,
	.ual_pause = smu_v15_0_8_ual_pause,
	.ual_resume = smu_v15_0_8_ual_resume,
	.ual_trigger_mode2 = smu_v15_0_8_ual_trigger_mode2,
};

static int smu_v15_0_8_gpumon_sw_init(struct amdgv_adapter *adapt)
{
	adapt->gpumon.funcs = &smu_v15_0_8_gpumon_funcs;

	adapt->i2c_cmd_buffer = oss_malloc(I2C_CMD_BUFFER_SIZE);
	if (adapt->i2c_cmd_buffer == OSS_INVALID_HANDLE) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_DRIVER_ALLOC_SYSTEM_MEM_FAIL,
			I2C_CMD_BUFFER_SIZE);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int smu_v15_0_8_gpumon_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->gpumon.funcs = NULL;
	oss_free(adapt->i2c_cmd_buffer);
	adapt->i2c_cmd_buffer = NULL;

	return 0;
}

static int smu_v15_0_8_gpumon_hw_init(struct amdgv_adapter *adapt)
{
	smu_v15_0_8_get_vbios_cache(adapt);

	return 0;
}

static int smu_v15_0_8_gpumon_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func smu_v15_0_8_gpumon_func = {
	.name = "smu_v15_0_8_gpumon_func",
	.sw_init = smu_v15_0_8_gpumon_sw_init,
	.sw_fini = smu_v15_0_8_gpumon_sw_fini,
	.hw_init = smu_v15_0_8_gpumon_hw_init,
	.hw_fini = smu_v15_0_8_gpumon_hw_fini,
};
