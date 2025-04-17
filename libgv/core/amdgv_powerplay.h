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

#ifndef AMDGV_POWERPLAY_H
#define AMDGV_POWERPLAY_H
#include "amdgv_powerplay_hwmgr.h"
#include "amdgv_common_eeprom.h"
#include "amdgv_gpumon.h"

struct amdgv_pp_metrics;
struct i2c_msg;
struct umc_ecc_info;

struct pp_pf_pci_state {
	uint32_t idx_vf;
	uint32_t *pci_cfg;
	uint32_t *msix_tab;
};

enum pp_powersaving_status{
	PP_POWERSAVING_STATUS_BACO_EXIT      = 0,
	PP_POWERSAVING_STATUS_D3HOT_OFF      = PP_POWERSAVING_STATUS_BACO_EXIT,
	PP_POWERSAVING_STATUS_IN_BACO        = 1,
	PP_POWERSAVING_STATUS_D3HOT_ON       = PP_POWERSAVING_STATUS_IN_BACO,
	PP_POWERSAVING_STATUS_D3HOT_ENTERING = 2,
	PP_POWERSAVING_STATUS_D3HOT_EXITING  = 3,
	PP_POWERSAVING_STATUS_D3HOT_BAMxCO_PHASE0_ENTRY = 4,
	PP_POWERSAVING_STATUS_D3HOT_BAMxCO_PHASE1_ENTRY = 5,
	PP_POWERSAVING_STATUS_D3HOT_MSR_ENTRY   = 6,
	PP_POWERSAVING_STATUS_D3HOT_MSR_EXIT    = 7,
	PP_POWERSAVING_STATUS_D3HOT_BAMxCO_EXIT = 8,
	PP_POWERSAVING_STATUS_MAX,
};

enum pp_clock_type {
	PP_CLOCK_TYPE__GFX,
	PP_CLOCK_TYPE__UCLK,
	PP_CLOCK_TYPE__SOC,
	PP_CLOCK_TYPE__VCLK,
	PP_CLOCK_TYPE__DCLK,
	PP_CLOCK_TYPE__DCEFCLK,
	PP_CLOCK_TYPE__DISPCLK,
	PP_CLOCK_TYPE__PIXCLK,
	PP_CLOCK_TYPE__PHYCLK,
	PP_CLOCK_TYPE__FCLK,
	PP_CLOCK_TYPE__VCLK_1,
	PP_CLOCK_TYPE__DCLK_1,
	PP_CLOCK_TYPE__DPPCLK,
	PP_CLOCK_TYPE__DPREFCLK,
	PP_CLOCK_TYPE__DCFCLK,
	PP_CLOCK_TYPE__DTBCLK,
	PP_CLOCK_TYPE__COUNT,
};

enum pp_clock_limit_type {
	PP_CLOCK_LIMIT_TYPE__SOFT_MIN,
	PP_CLOCK_LIMIT_TYPE__SOFT_MAX,
	PP_CLOCK_LIMIT_TYPE__COUNT,
};

enum pp_ras_type {
	PP_RAS_TYPE__POISON_COMSUMPTION = 1 << 1,
	PP_RAS_TYPE__FATAL_ERROR        = 1 << 2,
	PP_RAS_TYPE__RMA                = 1 << 7,
};

enum pp_throttler_event {
	PP_THROTTLER_EVENT__PROCHOT = 0,
	PP_THROTTLER_EVENT__SOCKET,
	PP_THROTTLER_EVENT__VR,
	PP_THROTTLER_EVENT__HBM,
	PP_THROTTLER_EVENT__COUNT,
};

enum pp_xgmi_plpd_mode {
	PP_XGMI_PLPD_MODE_DISABLE = 0,
	PP_XGMI_PLPD_MODE_ENABLE,
	PP_XGMI_PLPD_MODE_OPTIMIZED,
};

enum pp_df_cstate {
	DF_CSTATE_DISALLOW = 0,
	DF_CSTATE_ALLOW,
};

enum pp_rma_reason {
	PP_RMA_BAD_PAGE_THRESHOLD = 0,
};

struct amdgv_pp_funcs {
	int (*smu_init)(struct amdgv_adapter *adapt);
	int (*smu_fini)(struct amdgv_adapter *adapt);
	int (*pp_table_init)(struct amdgv_adapter *adapt);
	int (*pp_table_fini)(struct amdgv_adapter *adapt);
	int (*start_smu)(struct amdgv_adapter *adapt);
	int (*backend_init)(struct amdgv_adapter *adapt);
	int (*asic_setup)(struct amdgv_adapter *adapt);
	bool (*is_dpm_running)(struct amdgv_adapter *adapt);
	int (*dynamic_state_management_enable)(struct amdgv_adapter *adapt);
	int (*set_fan_control_mode)(struct amdgv_adapter *adapt);
	int (*get_fan_control_mode)(struct amdgv_adapter *adapt);
	int (*set_fan_speed_rpm)(struct amdgv_adapter *adapt);
	int (*get_fan_speed_rpm)(struct amdgv_adapter *adapt);
	int (*get_smu_fw_loaded_status)(struct amdgv_adapter *adapt);
	int (*wait_smu_idle)(struct amdgv_adapter *adapt, int timeout);
	int (*enter_baco)(struct amdgv_adapter *adapt);
	int (*exit_baco)(struct amdgv_adapter *adapt);
	int (*mode1_reset)(struct amdgv_adapter *adapt);
	int (*wait_mode1_reset_completion)(struct amdgv_adapter *adapt);
	int (*mode2_reset)(struct amdgv_adapter *adapt);
	int (*append_vbios_pptable)(struct amdgv_adapter *adapt);
	int (*get_pp_metrics)(struct amdgv_adapter *adapt,
			      struct amdgv_gpumon_metrics *metrics);
	int (*get_ecc_info)(struct amdgv_adapter *adapt,
			    struct umc_ecc_info *eccinfo);
	uint64_t (*metrics_table_size)(struct amdgv_adapter *adapt);
	uint64_t (*config_table_size)(struct amdgv_adapter *adapt);
	int (*set_driver_config)(struct amdgv_adapter *adapt);
	int (*get_clock_limit)(struct amdgv_adapter *adapt, enum pp_clock_type clk,
			       enum pp_clock_limit_type limit_type, uint32_t *freq);
	int (*get_power_capacity)(struct amdgv_adapter *adapt, int *val);
	int (*set_power_capacity)(struct amdgv_adapter *adapt, int val);
	int (*get_dpm_capacity)(struct amdgv_adapter *adapt, int *val);
	int (*i2c_eeprom_xfer)(struct amdgv_adapter *adapt, uint8_t port, struct i2c_msg *msgs,
			       int num);
	int (*get_fru_product_info)(struct amdgv_adapter *adapt);
	int (*control_fru_sigout)(struct amdgv_adapter *adapt, const uint8_t *passphrase);
	int (*send_hbm_bad_pages_num)(struct amdgv_adapter *adapt, uint32_t size);
	int (*send_hbm_bad_channel_flag)(struct amdgv_adapter *adapt, uint32_t size);
	int (*trigger_vf_flr)(struct amdgv_adapter *adapt, uint32_t param);
	int (*parse_smu_table_info)(struct amdgv_adapter *adapt);
	int (*get_clock_limit_driver_freq)(struct amdgv_adapter *adapt, enum pp_clock_type clk,
					enum pp_clock_limit_type limit_type, uint32_t *freq);
	int (*set_clock_limit_driver_freq)(struct amdgv_adapter *adapt, enum pp_clock_type clk,
					enum pp_clock_limit_type limit_type, uint32_t freq);
	int (*enter_power_saving)(struct amdgv_adapter *adapt);
	int (*exit_power_saving)(struct amdgv_adapter *adapt);
	int (*query_power_saving_status)(struct amdgv_adapter *adapt, uint32_t *status);
	int (*is_clock_locked)(struct amdgv_adapter *adapt, enum AMDGV_PP_CLK_DOMAIN clk_domain,
					uint8_t *clk_locked);
	int (*handle_smu_irq)(struct amdgv_adapter *adapt, struct amdgv_iv_entry *entry);
	int (*get_max_configurable_power_limit)(struct amdgv_adapter *adapt,
				int *power_limit);
	int (*get_default_power_limit)(struct amdgv_adapter *adapt,
				int *default_power);
	int (*get_metrics_ext)(struct amdgv_adapter *adapt,
			struct amdgv_gpumon_metrics_ext *metrics_ext);
	int (*get_num_metrics_ext_entries)(struct amdgv_adapter *adapt,
				 uint32_t *entries);
	int (*is_pm_enabled)(struct amdgv_adapter *adapt,
		bool *pm_enabled);
	int (*get_link_metrics)(struct amdgv_adapter *adapt,
			struct amdgv_gpumon_link_metrics *link_metrics);
	int (*set_df_cstate)(struct amdgv_adapter *adapt, enum pp_df_cstate state);
	int (*send_rma_reason)(struct amdgv_adapter *adapt, enum pp_rma_reason rma_reason);
	int (*get_shutdown_temperature)(struct amdgv_adapter *adapt, int *val);
	int (*prepare_unload)(struct amdgv_adapter *adapt);
	int (*set_workload_profile)(struct amdgv_adapter *adapt);
};

struct amdgv_pp_metrics_cache {
	void *metrics;
	uint64_t tstamp;
};

struct amdgv_pp {
	struct phm_platform_descriptor platform_descriptor;
	struct pp_thermal_controller_info thermal_controller;
	void *pptable;
	void *smu_backend;
	void *backend;
	void *drv_metrics_ext;
	struct amdgv_pp_metrics_cache metrics_cache;
	const void *soft_pp_table;
	uint32_t soft_pp_table_size;
	uint32_t available_fb_base;

	uint32_t pstate_sclk;
	uint32_t pstate_mclk;

	uint32_t fan_control_mode;
	uint32_t fan_rpm;

	uint32_t smu_fw_version;
	mutex_t smu_lock;

	uint32_t smu_features_mask[2];

	const struct amdgv_pp_funcs *pp_funcs;

	/* test SMU functionality, expect SMU failure */
	bool is_test_smu;

	bool is_in_powersaving;
	uint32_t pf_bif_bx_strap0;
	uint32_t pf_bif_db_int;
	struct pp_pf_pci_state pf_pci_config;

};

int amdgv_pp_early_init(struct amdgv_adapter *adapt);
int amdgv_pp_sw_init(struct amdgv_adapter *adapt);
int amdgv_pp_hw_init(struct amdgv_adapter *adapt);
int amdgv_pp_fini(struct amdgv_adapter *adapt);

#endif
