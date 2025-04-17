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

#ifndef MI300_POWERPLAY_H
#define MI300_POWERPLAY_H

#define AMD_MAX_USEC_TIMEOUT 200000 /* 200 ms */

#define MP0_Private 0x03700000
#define MP0_Public  0x03800000
#define MP0_SRAM    0x03900000
#define MP1_Private 0x03a00000
#define MP1_Public  0x03b00000
#define MP1_SRAM    0x03c00004

#define smnMP1_FIRMWARE_FLAGS 0x03010028

#define PAGE_SIZE 0x1000

#define TOOL_SIZE 0x19000

#define MI300_ENGINECLOCK_HARDMAX 198000

#define PP_FB_SIZE	   0x400000
#define PP_FB_START_OFFSET 0x800000

#define SMUQ10_TO_UINT(x) ((x) >> 10)
#define SMUQ16_TO_UINT(x) ((x) >> 16)
#define SMUQ10_FRAC(x) ((x) & 0x3ff)
#define SMUQ10_ROUND(x) ((SMUQ10_TO_UINT(x)) + ((SMUQ10_FRAC(x)) >= 0x200))

// This DS threshold was calculated based on MI300 characteristics using the formula
// max GFXCLK/GFX DS clock divider =  (2250/16) ~= 140
#define MI300_GPUMON_DS_THRESHOLD 140

struct mi300_smu_dpm_context;
struct mi300_smu_dpm_policy_ctxt;

struct amdgv_adapter;

struct mi300_smu_dpm_policy {
	enum amdgv_pp_pm_policy policy_type;
	struct smu_dpm_policy_desc policies[AMDGV_GPUMON_SOC_PSTATE_COUNT];
	uint32_t level_mask;
	int current_level;
	uint32_t num_supported;
	int (*set_policy)(struct amdgv_adapter *adapt, int level);
};

struct mi300_smu_dpm_policy_ctxt {
	struct mi300_smu_dpm_policy policies[AMDGV_PP_PM_POLICY_NUM];
	uint32_t policy_mask;
};

int mi300_smu_get_pm_policy(struct amdgv_adapter *adapt,
			    enum amdgv_pp_pm_policy p_type,
			    struct mi300_smu_dpm_policy **policy_int);
int mi300_smu_compare_and_set_pm_policy(struct amdgv_adapter *adapt,
					enum amdgv_pp_pm_policy p_type,
					int level);
int mi300_smu_error_inject_set_pm_policy(struct amdgv_adapter *adapt,
					 enum amdgv_pp_pm_policy p_type,
					 int level);
void mi300_smu_error_inject_restore_pm_policy(struct amdgv_adapter *adapt,
					     enum amdgv_pp_pm_policy p_type);


bool mi300_smu_get_fw_loaded_status(struct amdgv_adapter *adapt);
int mi300_gpu_mode1_reset(struct amdgv_adapter *adapt);
int mi300_wait_gpu_reset_completion(struct amdgv_adapter *adapt);

uint32_t mi300_smu_read_arg(struct amdgv_adapter *adapt);
int mi300_smu_send_msg_with_param(struct amdgv_adapter *adapt, uint32_t msg, uint32_t param,
				  uint32_t *arg);
int mi300_smu_send_msg(struct amdgv_adapter *adapt, uint32_t msg, uint32_t *arg);
int mi300_smu_get_version(struct amdgv_adapter *adapt, uint32_t *smu_version,
			  uint32_t *driver_if_version);
int mi300_smu_get_metrics_version(struct amdgv_adapter *adapt, uint32_t *version);
int mi300_smu_mca_set_debug_mode(struct amdgv_adapter *adapt, bool enable);
int mi300_smu_trigger_vf_flr(struct amdgv_adapter *adapt, uint32_t idx_vf);
int mi300_smu_trigger_mode_3_reset(struct amdgv_adapter *adapt, uint32_t xcc_mask);
int mi300_smu_gfx_flr_recovery(struct amdgv_adapter *adapt, uint32_t idx_vf);
int mi300_smu_gfx_mode_3_recovery(struct amdgv_adapter *adapt, uint32_t xcc_mask);
#endif
