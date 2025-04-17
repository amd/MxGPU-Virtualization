/*
 * Copyright (c) 2017-2021 Advanced Micro Devices, Inc. All rights reserved.
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

#ifndef AMDGV_GPUMON_INTERNAL_H
#define AMDGV_GPUMON_INTERNAL_H

#include "amdgv_basetypes.h"
#include "amdgv_api.h"
#include "amdgv_gpumon.h"

enum amdgv_gpumon_type {
	/* GETTERS */
	GPUMON_GET_ASIC_TEMP,
	GPUMON_GET_ALL_TEMP,
	GPUMON_GET_GPU_POWER_USAGE,
	GPUMON_GET_GPU_POWER_CAP,
	GPUMON_SET_GPU_POWER_CAP,
	GPUMON_GET_VDDC,
	GPUMON_GET_DPM_STATUS,
	GPUMON_GET_PCIE_CONFS,
	GPUMON_GET_DPM_CAP,
	GPUMON_GET_GFX_ACT,
	GPUMON_GET_MEM_ACT,
	GPUMON_GET_UVD_ACT,
	GPUMON_GET_VCE_ACT,
	GPUMON_GET_ECC_INFO,
	GPUMON_GET_PP_METRICS,
	GPUMON_GET_SCLK,
	GPUMON_GET_MAX_SCLK,
	GPUMON_GET_MAX_MCLK,
	GPUMON_GET_MAX_VCLK0,
	GPUMON_GET_MAX_VCLK1,
	GPUMON_GET_MAX_DCLK0,
	GPUMON_GET_MAX_DCLK1,
	GPUMON_GET_MIN_SCLK,
	GPUMON_GET_MIN_VCLK0,
	GPUMON_GET_MIN_VCLK1,
	GPUMON_GET_MIN_DCLK0,
	GPUMON_GET_MIN_DCLK1,
	GPUMON_GET_MIN_MCLK,
	GPUMON_GET_FW_ERR_RECORDS,
	GPUMON_GET_VF_FW_INFO,
	GPUMON_GET_VRAM_INFO,
	GPUMON_GET_ACCELERATOR_PARTITION_PROFILE,
	GPUMON_GET_MEMORY_PARTITION_MODE,
	GPUMON_GET_SPATIAL_PARTITION_NUM,
	GPUMON_GET_PCIE_REPLAY_COUNT,
	GPUMON_GET_CARD_FORM_FACTOR,
	GPUMON_GET_MAX_CONFIG_POWER_LIMIT,
	GPUMON_GET_DEFAULT_POWER_LIMIT,
	GPUMON_GET_MIN_POWER_LIMIT,
	GPUMON_GET_METRICS_EXT,
	GPUMON_GET_NUM_METRICS_EXT_ENTRIES,
	GPUMON_GET_LINK_METRICS,
	GPUMON_GET_LINK_TOPOLOGY,
	GPUMON_GET_XGMI_FB_SHARING_CAPS,
	GPUMON_GET_XGMI_FB_SHARING_MODE_INFO,
	GPUMON_GET_SHUTDOWN_TEMP,
	GPUMON_GET_GPU_CACHE_INFO,
	GPUMON_GET_MAX_PCIE_LINK_GENERATION,
	GPUMON_GET_FRU_PRODUCT_INFO,
	GPUMON_GET_DPM_POLICY_LEVEL,
	GPUMON_GET_XGMI_PLPD_POLICY_LEVEL,
	GPUMON_IS_POWER_MANAGEMENT_ENABLED,
	GPUMON_IS_CLK_LOCKED,
	GPUMON_RAS_REPORT,
	GPUMON_CPER_GET_COUNT,
	GPUMON_CPER_GET_ENTRIES,

	/* -- SETTERS -- */

	GPUMON_FRU_SIGOUT,
	GPUMON_RAS_EEPROM_CLEAR,
	GPUMON_RAS_ERROR_INJECT,
	GPUMON_TURN_ON_ECC_INJECTION,
	GPUMON_ALLOC_VF,
	GPUMON_SET_VF_NUM,
	GPUMON_FREE_VF,
	GPUMON_SET_VF,
	GPUMON_CLEAN_CORRECTABLE_ERROR_COUNT,
	GPUMON_SET_ACCELERATOR_PARTITION_PROFILE,
	GPUMON_SET_MEMORY_PARTITION_MODE,
	GPUMON_SET_SPATIAL_PARTITION_NUM,
	GPUMON_RESET_SPATIAL_PARTITION_NUM,
	GPUMON_SET_XGMI_FB_SHARING_MODE,
	GPUMON_SET_XGMI_FB_SHARING_MODE_EX,
	GPUMON_RAS_TA_LOAD,
	GPUMON_RAS_TA_UNLOAD,
	GPUMON_RESET_ALL_ERROR_COUNTS,
	GPUMON_SET_PM_POLICY_LEVEL,
	GPUMON_GET_GFX_CONFIG,
	GPUMON_MAX_TYPE
};

extern enum amdgv_gpumon_type gpumon_unrecov_err_whitelist[];
extern uint32_t gpumon_unrecov_err_whitelist_len;

struct amdgv_gpumon_funcs {
	int (*get_gpu_power_usage)(struct amdgv_adapter *adapt, int *val);
	int (*get_gpu_power_capacity)(struct amdgv_adapter *adapt, int *val);
	int (*set_gpu_power_capacity)(struct amdgv_adapter *adapt, int val);
	int (*get_dpm_status)(struct amdgv_adapter *adapt, int *val);
	int (*get_dpm_cap)(struct amdgv_adapter *adapt, int *val);
	int (*get_vddc)(struct amdgv_adapter *adapt, int *val);

	int (*get_asic_temperature)(struct amdgv_adapter *adapt, int *val);
	int (*get_hotspot_temperature)(struct amdgv_adapter *adapt, int *val);
	int (*get_mem_temperature)(struct amdgv_adapter *adapt, int *val);
	int (*get_plx_temperature)(struct amdgv_adapter *adapt, int *val);
	int (*get_all_temperature)(struct amdgv_adapter *adapt, struct amdgv_gpumon_temp *tmp);

	int (*get_gfx_activity)(struct amdgv_adapter *adapt, int *val);
	int (*get_mem_activity)(struct amdgv_adapter *adapt, int *val);
	int (*get_uvd_activity)(struct amdgv_adapter *adapt, int *val);
	int (*get_vce_activity)(struct amdgv_adapter *adapt, int *val);

	int (*get_sclk)(struct amdgv_adapter *adapt, int *val);
	int (*get_mclk)(struct amdgv_adapter *adapt, int *val);

	int (*get_max_sclk)(struct amdgv_adapter *adapt, int *val);
	int (*get_max_mclk)(struct amdgv_adapter *adapt, int *val);
	int (*get_max_dclk0)(struct amdgv_adapter *adapt, int *val);
	int (*get_max_vclk0)(struct amdgv_adapter *adapt, int *val);
	int (*get_max_dclk1)(struct amdgv_adapter *adapt, int *val);
	int (*get_max_vclk1)(struct amdgv_adapter *adapt, int *val);

	int (*get_min_sclk)(struct amdgv_adapter *adapt, int *val);
	int (*get_min_mclk)(struct amdgv_adapter *adapt, int *val);
	int (*get_min_dclk0)(struct amdgv_adapter *adapt, int *val);
	int (*get_min_vclk0)(struct amdgv_adapter *adapt, int *val);
	int (*get_min_dclk1)(struct amdgv_adapter *adapt, int *val);
	int (*get_min_vclk1)(struct amdgv_adapter *adapt, int *val);

	int (*get_avg_sclk)(struct amdgv_adapter *adapt, int *val);
	int (*get_avg_mclk)(struct amdgv_adapter *adapt, int *val);

	int (*get_gecc)(struct amdgv_adapter *adapt, uint32_t *val);
	int (*get_ecc_info)(struct amdgv_adapter *adapt, int *corr, int *uncorr);
	int (*clean_correctable_error_count)(struct amdgv_adapter *adapt, int *corr);
	int (*ras_report)(struct amdgv_adapter *adapt, int ras_type);

	int (*get_vbios_info)(struct amdgv_adapter *adapt,
			      struct amdgv_vbios_info *vbios_info);
	int (*get_vbios_cache)(struct amdgv_adapter *adapt);
	int (*get_pp_metrics)(struct amdgv_adapter *adapt,
			      struct amdgv_gpumon_metrics *metrics);
	int (*get_vram_info)(struct amdgv_adapter *adapt,
			     struct amdgv_gpumon_vram_info *vram_info);
	int (*is_clk_locked)(struct amdgv_adapter *adapt,
			     enum AMDGV_PP_CLK_DOMAIN clk_domain,
			     uint8_t *clk_locked);
	int (*get_memory_partition_config)(
		struct amdgv_adapter *adapt,
		union amdgv_gpumon_memory_partition_config
			*memory_partition_config);
	int (*get_spatial_partition_caps)(
		struct amdgv_adapter *adapt,
		struct amdgv_gpumon_spatial_partition_caps
			*spatial_partition_caps);
	int (*get_accelerator_partition_profile_config_global)(
		struct amdgv_adapter *adapt,
		struct amdgv_gpumon_accelerator_partition_profile_config
			*profile_configs);
	int (*get_accelerator_partition_profile_config)(
		struct amdgv_adapter *adapt,
		struct amdgv_gpumon_accelerator_partition_profile_config
			*profile_configs);
	int (*set_accelerator_partition_profile)(struct amdgv_adapter *adapt,
						 uint32_t profile_index);
	int (*get_accelerator_partition_profile)(
		struct amdgv_adapter *adapt,
		struct amdgv_gpumon_acccelerator_partition_profile
			*accelerator_partition_profile);
	int (*set_spatial_partition_num)(struct amdgv_adapter *adapt,
					 uint32_t spatial_partition_num);
	int (*set_memory_partition_mode)(
		struct amdgv_adapter *adapt,
		enum amdgv_memory_partition_mode memory_partition_mode);
	int (*reset_spatial_partition_num)(struct amdgv_adapter *adapt);
	int (*get_memory_partition_mode)(
		struct amdgv_adapter *adapt,
		struct amdgv_gpumon_memory_partition_info *memory_partition_info);
	int (*get_spatial_partition_num)(struct amdgv_adapter *adapt,
					 uint32_t *spatial_partition_num);
	int (*get_pcie_replay_count)(struct amdgv_adapter *adapt, int *val);
	int (*get_card_form_factor)(
		struct amdgv_adapter *adapt,
		enum amdgv_gpumon_card_form_factor *card_form_factor);
	int (*get_max_configurable_power_limit)(struct amdgv_adapter *adapt,
				int *power_limit);
	int (*get_default_power_limit)(struct amdgv_adapter *adapt,
				int *default_power);
	int (*get_min_power_limit)(struct amdgv_adapter *adapt,
				int *min_power_limit);
	int (*get_metrics_ext)(struct amdgv_adapter *adapt,
			       struct amdgv_gpumon_metrics_ext *metrics_ext);
	int (*get_num_metrics_ext_entries)(struct amdgv_adapter *adapt,
					   uint32_t *entries);
	int (*is_power_management_enabled)(struct amdgv_adapter *adapt,
					   bool *pm_enabled);
	int (*get_link_metrics)(struct amdgv_adapter *adapt,
			struct amdgv_gpumon_link_metrics *link_metrics);
	int (*get_link_topology)(struct amdgv_adapter *adapt,
			struct amdgv_adapter *dest_adapt,
			struct amdgv_gpumon_link_topology_info *topology_info);
	int (*get_xgmi_fb_sharing_caps)(struct amdgv_adapter *adapt,
			union amdgv_gpumon_xgmi_fb_sharing_caps *caps);
	int (*get_xgmi_fb_sharing_mode_info)(struct amdgv_adapter *src_adapt,
			struct amdgv_adapter *dest_adapt, enum amdgv_gpumon_xgmi_fb_sharing_mode mode,
			uint8_t *is_sharing_enabled);
	int (*set_xgmi_fb_sharing_mode)(struct amdgv_adapter *adapt,
			enum amdgv_gpumon_xgmi_fb_sharing_mode mode);
	int (*set_xgmi_fb_sharing_mode_ex)(struct amdgv_adapter *adapt,
			enum amdgv_gpumon_xgmi_fb_sharing_mode mode,
			uint32_t sharing_mask);
	int (*get_shutdown_temperature)(struct amdgv_adapter *adapt,
			int *shutdown_temp);
	int (*get_gpu_cache_info)(struct amdgv_adapter *adapt,
			struct amdgv_gpumon_gpu_cache_info *gpu_cache_info);
	int (*get_max_pcie_link_generation)(struct amdgv_adapter *adapt,
			int *val);
	int (*set_pm_policy_level)(struct amdgv_adapter *adapt,
			enum amdgv_pp_pm_policy p_type,
			enum amdgv_pp_policy_soc_pstate level);
	int (*get_pm_policy)(struct amdgv_adapter *adapt,
			enum amdgv_pp_pm_policy p_type,
			struct amdgv_gpumon_smu_dpm_policy *policy);
	int (*get_gfx_config)(struct amdgv_adapter *adapt,
			struct amdgv_gpumon_gfx_config *config);
};

struct amdgv_gpumon {
	uint32_t dev_id;
	uint32_t rev_id;
	const struct amdgv_gpumon_funcs *funcs;
};

void amdgv_gpumon_update_load_start_time(struct amdgv_adapter *adapt, uint32_t idx_vf,
					 struct amdgv_sched_world_switch *world_switch, bool is_init);
void amdgv_gpumon_update_save_end_time(struct amdgv_adapter *adapt, uint32_t idx_vf,
				       struct amdgv_sched_world_switch *world_switch);
int amdgv_gpumon_handle_sched_event(struct amdgv_adapter *adapt,
				    struct amdgv_sched_event *event);

int amdgv_vf_get_option_type(struct amdgv_adapter *adapt, enum amdgv_set_vf_opt_type *opt_type,
			     struct amdgv_vf_option *opt);

int amdgv_set_accelerator_partition_profile(struct amdgv_adapter *adapt,
	    uint32_t profile_index);

int amdgv_set_memory_partition_mode(struct amdgv_adapter *adapt,
		enum amdgv_memory_partition_mode memory_partition_mode);

#endif
