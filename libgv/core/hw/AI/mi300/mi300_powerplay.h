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

#define PLDM_VERSION_NOT_SUPPORTED 0xffffffff

// This DS threshold was calculated based on MI300 characteristics using the formula
// max GFXCLK/GFX DS clock divider =  (2250/16) ~= 140
#define MI300_GPUMON_DS_THRESHOLD 140

enum mi300_metric_name {
	MI300_METRICS_COUNTER,
	MI300_GFX_CLK_XCD,
	MI300_GFX_CLK_MAX_XCD,
	MI300_GFX_CLK_MIN_XCD,
	MI300_GFX_CLK_DS_XCD,
	MI300_GFX_CLK_LOCKED_XCD,
	MI300_SOC_CLK_AID,
	MI300_SOC_CLK_MAX_AID,
	MI300_SOC_CLK_MIN_AID,
	MI300_SOC_CLK_DS_AID,
	MI300_VCLK_AID,
	MI300_VCLK_MAX_AID,
	MI300_VCLK_MIN_AID,
	MI300_VCLK_DS_AID,
	MI300_DCLK_AID,
	MI300_DCLK_MAX_AID,
	MI300_DCLK_MIN_AID,
	MI300_DCLK_DS_AID,
	MI300_MEMCLK,
	MI300_MEMCLK_MAX,
	MI300_MEMCLK_MIN,
	MI300_MEM_CLK_DS,
	MI300_GFX_CLK_XCD_ACC,
	MI300_USAGE_GFX_XCD,
	MI300_USAGE_GFX,
	MI300_USAGE_MEM,
	MI300_USAGE_JPEG,
	MI300_USAGE_VCN,
	MI300_DRAM_BANDWIDTH_MAX,
	MI300_USAGE_GFX_XCD_ACC,
	MI300_USAGE_GFX_ACC,
	MI300_USAGE_MEM_ACC,
	MI300_DRAM_BANDWIDTH_ACC,
	MI300_TEMP_HOTSPOT,
	MI300_TEMP_VR,
	MI300_TEMP_MEM,
	MI300_TEMP_HOTSPOT_ACC,
	MI300_TEMP_VR_ACC,
	MI300_TEMP_MEM_ACC,
	MI300_POWER_MAX,
	MI300_POWER,
	MI300_ENERGY_SOCKET_ACC,
	MI300_ENERGY_XCD_ACC,
	MI300_ENERGY_AID_ACC,
	MI300_ENERGY_MEM_ACC,
	MI300_THROT_GFX_BEL_PPT_ACC,
	MI300_THROT_GFX_BEL_THM_ACC,
	MI300_THROT_GFX_BEL_TOT_ACC,
	MI300_THROT_GFX_CLK_LOW_ACC,
	MI300_THROT_PROCHOT_ACC,
	MI300_THROT_PPT_ACC,
	MI300_THROT_SOCKET_ACC,
	MI300_THROT_VR_ACC,
	MI300_THROT_MEM_ACC,
	MI300_PCIE_LINK_SPEED,
	MI300_PCIE_LINK_WIDTH,
	MI300_PCIE_BANDWIDTH,
	MI300_PCIE_BANDWIDTH_ACC,
	MI300_PCIE_L0_TO_RECOVER_ACC,
	MI300_PCIE_REPL_ACC,
	MI300_PCIE_REPL_ROLLOVER_ACC,
	MI300_PCIE_NAK_SENT_ACC,
	MI300_PCIE_NAK_RECEIVED_ACC,
	MI300_IN_TEL_VOLTAGE,
	MI300_PLDM_VERSION,
	MI300_METRIC_NAME_COUNT,
};

struct amdgv_adapter;

bool mi300_smu_get_fw_loaded_status(struct amdgv_adapter *adapt);
int mi300_gpu_mode1_reset(struct amdgv_adapter *adapt, bool is_unload);
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
