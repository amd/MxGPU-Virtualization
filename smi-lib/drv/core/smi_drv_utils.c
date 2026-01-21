/*
 * Copyright (c) 2024-2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include "smi_drv_utils.h"

uint64_t smi_eeprom_to_utc_format(uint64_t eeprom_timestamp)
{
	uint64_t year, month, day, hour, minute, second;
	uint64_t i;
	uint64_t utc_timestamp = 0;
	int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

	second = eeprom_timestamp & 0x3F;
	minute = (eeprom_timestamp >> EEPROM_TIMESTAMP_MINUTE) & 0x3F;
	hour = (eeprom_timestamp >> EEPROM_TIMESTAMP_HOUR) & 0x1F;
	day = (eeprom_timestamp >> EEPROM_TIMESTAMP_DAY) & 0x3F;
	month = (eeprom_timestamp >> EEPROM_TIMESTAMP_MONTH) & 0x0F;
	year = (eeprom_timestamp >> EEPROM_TIMESTAMP_YEAR) & 0x1F;

	year += 2000;

	/* Validate extracted values to prevent array bounds violations and invalid dates */
	if (month < 1 || month > 12 ||
		day < 1 || day > 31 ||
		hour > 23 || minute > 59 || second > 59) {
		/* Invalid date/time, return -1 */
		return SMI_NOT_SUPPORTED;
	}

	for (i = 1970; i < year; i++) {
		utc_timestamp += (IS_LEAP_YEAR(i) ? 366 : 365) * 24 * 60 * 60;
	}

	for (i = 1; i < month; i++) {
		utc_timestamp += days_in_month[i - 1] * 24 * 60 * 60;
		if (i == 2 && IS_LEAP_YEAR(year))
			utc_timestamp += 24 * 60 * 60;
	}

	utc_timestamp += (day - 1) * 24 * 60 * 60;
	utc_timestamp += hour * 60 * 60;
	utc_timestamp += minute * 60;
	utc_timestamp += second;

	return utc_timestamp;
}

uint64_t smi_eeprom_v4_to_utc_format(uint64_t eeprom_timestamp)
{
	uint64_t year, month, day, hour, minute, second;
	uint64_t i;
	uint64_t utc_timestamp = 0;
	int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	uint32_t ts_hi, ts_lo;
	uint8_t raw_val;

	//V4 format uses the following split format:
	// ts_hi: yy[31:16] mm[15:8] day[7:0]
	// ts_lo: hh[23:16] mm[15:8] ss[7:0]
	ts_hi = (uint32_t)(eeprom_timestamp >> 32);  // Upper 32 bits
	ts_lo = (uint32_t)(eeprom_timestamp & 0xFFFFFFFF);  // Lower 32 bits

	raw_val = (ts_hi >> EEPROM_V4_TIMESTAMP_YEAR) & 0xFF;
	year = 2000 + raw_val;

	month = (ts_hi >> EEPROM_V4_TIMESTAMP_MONTH) & 0xFF;
	day = ts_hi & 0xFF;
	hour = (ts_lo >> EEPROM_V4_TIMESTAMP_HOUR) & 0xFF;
	minute = (ts_lo >> EEPROM_V4_TIMESTAMP_MINUTE) & 0xFF;
	second = ts_lo & 0xFF;

	/* Validate extracted values to prevent array bounds violations and invalid dates */
	if (month < 1 || month > 12 ||
		day < 1 || day > 31 ||
		hour > 23 || minute > 59 || second > 59) {
		/* Invalid date/time, return -1 */
		return SMI_NOT_SUPPORTED;
	}

	for (i = 1970; i < year; i++) {
		utc_timestamp += (IS_LEAP_YEAR(i) ? 366 : 365) * 24 * 60 * 60;
	}

	for (i = 1; i < month; i++) {
		utc_timestamp += days_in_month[i - 1] * 24 * 60 * 60;
		if (i == 2 && IS_LEAP_YEAR(year)) {
			utc_timestamp += 24 * 60 * 60;
		}
	}

	utc_timestamp += (day - 1) * 24 * 60 * 60;
	utc_timestamp += hour * 60 * 60;
	utc_timestamp += minute * 60;
	utc_timestamp += second;

	return utc_timestamp;
}

void smi_generate_time_string(char *buf, uint64_t microsec)
{
	uint64_t hour, min, sec, millisec;

	millisec = (microsec % 1000000) / 1000;

	/* convert to sec */
	sec = microsec / 1000000;

	hour = sec / (60*60);
	sec  = sec % (60*60);
	min  = sec / 60;
	sec  = sec % 60;

	smi_vsnprintf(buf, SMI_MAX_DATE_LENGTH,
		SMI_TIME_FORMAT,
		(int) hour,
		(int) min,
		(int) sec,
		(int) millisec);

	/* HH:MM:SS.MSC */
	buf[12] = 0;
}

enum smi_fw_block smi_ucode_amdgv_to_smi(enum amdgv_firmware_id ucode_id)
{
	uint32_t ret_ucode_id;

	switch (ucode_id) {
	case AMDGV_FIRMWARE_ID__SMU:
		ret_ucode_id = SMI_FW_ID_SMU;
		break;
	case AMDGV_FIRMWARE_ID__CP_CE:
		ret_ucode_id = SMI_FW_ID_CP_CE;
		break;
	case AMDGV_FIRMWARE_ID__CP_PFP:
		ret_ucode_id = SMI_FW_ID_CP_PFP;
		break;
	case AMDGV_FIRMWARE_ID__CP_ME:
		ret_ucode_id = SMI_FW_ID_CP_ME;
		break;
	case AMDGV_FIRMWARE_ID__CP_MEC_JT1:
		ret_ucode_id = SMI_FW_ID_CP_MEC_JT1;
		break;
	case AMDGV_FIRMWARE_ID__CP_MEC_JT2:
		ret_ucode_id = SMI_FW_ID_CP_MEC_JT2;
		break;
	case AMDGV_FIRMWARE_ID__CP_MEC1:
		ret_ucode_id = SMI_FW_ID_CP_MEC1;
		break;
	case AMDGV_FIRMWARE_ID__CP_MEC2:
		ret_ucode_id = SMI_FW_ID_CP_MEC2;
		break;
	case AMDGV_FIRMWARE_ID__RLC:
		ret_ucode_id = SMI_FW_ID_RLC;
		break;
	case AMDGV_FIRMWARE_ID__SDMA0:
		ret_ucode_id = SMI_FW_ID_SDMA0;
		break;
	case AMDGV_FIRMWARE_ID__SDMA1:
		ret_ucode_id = SMI_FW_ID_SDMA1;
		break;
	case AMDGV_FIRMWARE_ID__SDMA2:
		ret_ucode_id = SMI_FW_ID_SDMA2;
		break;
	case AMDGV_FIRMWARE_ID__SDMA3:
		ret_ucode_id = SMI_FW_ID_SDMA3;
		break;
	case AMDGV_FIRMWARE_ID__SDMA4:
		ret_ucode_id = SMI_FW_ID_SDMA4;
		break;
	case AMDGV_FIRMWARE_ID__SDMA5:
		ret_ucode_id = SMI_FW_ID_SDMA5;
		break;
	case AMDGV_FIRMWARE_ID__SDMA6:
		ret_ucode_id = SMI_FW_ID_SDMA6;
		break;
	case AMDGV_FIRMWARE_ID__SDMA7:
		ret_ucode_id = SMI_FW_ID_SDMA7;
		break;
	case AMDGV_FIRMWARE_ID__VCN:
		ret_ucode_id = SMI_FW_ID_VCN;
		break;
	case AMDGV_FIRMWARE_ID__UVD:
		ret_ucode_id = SMI_FW_ID_UVD;
		break;
	case AMDGV_FIRMWARE_ID__VCE:
		ret_ucode_id = SMI_FW_ID_VCE;
		break;
	case AMDGV_FIRMWARE_ID__ISP:
		ret_ucode_id = SMI_FW_ID_ISP;
		break;
	case AMDGV_FIRMWARE_ID__DMCU_ERAM:
		ret_ucode_id = SMI_FW_ID_DMCU_ERAM;
		break;
	case AMDGV_FIRMWARE_ID__DMCU_ISR:
		ret_ucode_id = SMI_FW_ID_DMCU_ISR;
		break;
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_GPM_MEM:
		ret_ucode_id = SMI_FW_ID_RLC_RESTORE_LIST_GPM_MEM;
		break;
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_SRM_MEM:
		ret_ucode_id = SMI_FW_ID_RLC_RESTORE_LIST_SRM_MEM;
		break;
	case AMDGV_FIRMWARE_ID__RLC_RESTORE_LIST_CNTL:
		ret_ucode_id = SMI_FW_ID_RLC_RESTORE_LIST_CNTL;
		break;
	case AMDGV_FIRMWARE_ID__RLC_V:
		ret_ucode_id = SMI_FW_ID_RLC_V;
		break;
	case AMDGV_FIRMWARE_ID__MMSCH:
		ret_ucode_id = SMI_FW_ID_MMSCH;
		break;
	case AMDGV_FIRMWARE_ID__PSP_SYS:
		ret_ucode_id = SMI_FW_ID_PSP_SYSDRV;
		break;
	case AMDGV_FIRMWARE_ID__PSP_SOS:
		ret_ucode_id = SMI_FW_ID_PSP_SOSDRV;
		break;
	case AMDGV_FIRMWARE_ID__PSP_TOC:
		ret_ucode_id = SMI_FW_ID_PSP_TOC;
		break;
	case AMDGV_FIRMWARE_ID__PSP_KEYDB:
		ret_ucode_id = SMI_FW_ID_PSP_KEYDB;
		break;
	case AMDGV_FIRMWARE_ID__DFC_FW:
		ret_ucode_id = SMI_FW_ID_DFC;
		break;
	case AMDGV_FIRMWARE_ID__PSP_SPL:
		ret_ucode_id = SMI_FW_ID_PSP_SPL;
		break;
	case AMDGV_FIRMWARE_ID__DRV_CAP:
		ret_ucode_id = SMI_FW_ID_DRV_CAP;
		break;
	case AMDGV_FIRMWARE_ID__PSP_BL:
		ret_ucode_id = SMI_FW_ID_PSP_BL;
		break;
	case AMDGV_FIRMWARE_ID__RLC_P:
		ret_ucode_id = SMI_FW_ID_RLC_P;
		break;
	case AMDGV_FIRMWARE_ID__SEC_POLICY_STAGE2:
		ret_ucode_id = SMI_FW_ID_SEC_POLICY_STAGE2;
		break;
	case AMDGV_FIRMWARE_ID__REG_ACCESS_WHITELIST:
		ret_ucode_id = SMI_FW_ID_REG_ACCESS_WHITELIST;
		break;
	case AMDGV_FIRMWARE_ID__IMU_DRAM:
		ret_ucode_id = SMI_FW_ID_IMU_DRAM;
		break;
	case AMDGV_FIRMWARE_ID__IMU_IRAM:
		ret_ucode_id = SMI_FW_ID_IMU_IRAM;
		break;
	case AMDGV_FIRMWARE_ID__SDMA_UCODE_TH0:
		ret_ucode_id = SMI_FW_ID_SDMA_TH0;
		break;
	case AMDGV_FIRMWARE_ID__SDMA_UCODE_TH1:
		ret_ucode_id = SMI_FW_ID_SDMA_TH1;
		break;
	case AMDGV_FIRMWARE_ID__CP_MES:
		ret_ucode_id = SMI_FW_ID_CP_MES;
		break;
	case AMDGV_FIRMWARE_ID__MES_STACK:
		ret_ucode_id = SMI_FW_ID_MES_STACK;
		break;
	case AMDGV_FIRMWARE_ID__MES_THREAD1:
		ret_ucode_id = SMI_FW_ID_MES_THREAD1;
		break;
	case AMDGV_FIRMWARE_ID__MES_THREAD1_STACK:
		ret_ucode_id = SMI_FW_ID_MES_THREAD1_STACK;
		break;
	case AMDGV_FIRMWARE_ID__RLX6:
		ret_ucode_id = SMI_FW_ID_RLX6;
		break;
	case AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT:
		ret_ucode_id = SMI_FW_ID_RLX6_DRAM_BOOT;
		break;
	case AMDGV_FIRMWARE_ID__RS64_ME_UCODE:
		ret_ucode_id = SMI_FW_ID_RS64_ME;
		break;
	case AMDGV_FIRMWARE_ID__RS64_ME_P0_DATA:
		ret_ucode_id = SMI_FW_ID_RS64_ME_P0_DATA;
		break;
	case AMDGV_FIRMWARE_ID__RS64_ME_P1_DATA:
		ret_ucode_id = SMI_FW_ID_RS64_ME_P1_DATA;
		break;
	case AMDGV_FIRMWARE_ID__RS64_PFP_UCODE:
		ret_ucode_id = SMI_FW_ID_RS64_PFP;
		break;
	case AMDGV_FIRMWARE_ID__RS64_PFP_P0_DATA:
		ret_ucode_id = SMI_FW_ID_RS64_PFP_P0_DATA;
		break;
	case AMDGV_FIRMWARE_ID__RS64_PFP_P1_DATA:
		ret_ucode_id = SMI_FW_ID_RS64_PFP_P1_DATA;
		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_UCODE:
		ret_ucode_id = SMI_FW_ID_RS64_MEC;
		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_P0_DATA:
		ret_ucode_id = SMI_FW_ID_RS64_MEC_P0_DATA;
		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_P1_DATA:
		ret_ucode_id = SMI_FW_ID_RS64_MEC_P1_DATA;
		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_P2_DATA:
		ret_ucode_id = SMI_FW_ID_RS64_MEC_P2_DATA;
		break;
	case AMDGV_FIRMWARE_ID__RS64_MEC_P3_DATA:
		ret_ucode_id = SMI_FW_ID_RS64_MEC_P3_DATA;
		break;
	case AMDGV_FIRMWARE_ID__PPTABLE:
		ret_ucode_id = SMI_FW_ID_PPTABLE;
		break;
	case AMDGV_FIRMWARE_ID__PSP_SOC:
		ret_ucode_id = SMI_FW_ID_PSP_SOC;
		break;
	case AMDGV_FIRMWARE_ID__PSP_DBG:
		ret_ucode_id = SMI_FW_ID_PSP_DBG;
		break;
	case AMDGV_FIRMWARE_ID__PSP_INTF:
		ret_ucode_id = SMI_FW_ID_PSP_INTF;
		break;
	case AMDGV_FIRMWARE_ID__RLX6_UCODE_CORE1:
		ret_ucode_id = SMI_FW_ID_RLX6_CORE1;
		break;
	case AMDGV_FIRMWARE_ID__RLX6_DRAM_BOOT_CORE1:
		ret_ucode_id = SMI_FW_ID_RLX6_DRAM_BOOT_CORE1;
		break;
	case AMDGV_FIRMWARE_ID__RLCV_LX7:
		ret_ucode_id = SMI_FW_ID_RLCV_LX7;
		break;
	case AMDGV_FIRMWARE_ID__RLC_SAVE_RESTROE_LIST:
		ret_ucode_id = SMI_FW_ID_RLC_SAVE_RESTORE_LIST;
		break;
	case AMDGV_FIRMWARE_ID__PSP_RAS:
		ret_ucode_id = SMI_FW_ID_PSP_RAS;
		break;
	case AMDGV_FIRMWARE_ID__RAS_TA:
		ret_ucode_id = SMI_FW_ID_TA_RAS;
		break;
	case AMDGV_FIRMWARE_ID__P2S_TABLE:
		ret_ucode_id = SMI_FW_ID_P2S_TABLE;
		break;
	case AMDGV_FIRMWARE_ID__PLDM_VERSION:
		ret_ucode_id = SMI_FW_ID_PLDM_VERSION;
		break;
	default:
		ret_ucode_id = SMI_FW_ID__MAX;
		break;
	}

	return ret_ucode_id;
}

enum amdgv_gpumon_xgmi_fb_sharing_mode smi_map_fb_sharing_mode(enum smi_xgmi_fb_sharing_mode mode)
{
	uint32_t fb_sharing_mode;

	switch (mode) {
	case SMI_XGMI_FB_SHARING_MODE_CUSTOM:
		fb_sharing_mode = AMDGV_GPUMON_XGMI_FB_SHARING_MODE_CUSTOM;
		break;
	case SMI_XGMI_FB_SHARING_MODE_1:
		fb_sharing_mode = AMDGV_GPUMON_XGMI_FB_SHARING_MODE_1;
		break;
	case SMI_XGMI_FB_SHARING_MODE_2:
		fb_sharing_mode = AMDGV_GPUMON_XGMI_FB_SHARING_MODE_2;
		break;
	case SMI_XGMI_FB_SHARING_MODE_4:
		fb_sharing_mode = AMDGV_GPUMON_XGMI_FB_SHARING_MODE_4;
		break;
	case SMI_XGMI_FB_SHARING_MODE_8:
		fb_sharing_mode = AMDGV_GPUMON_XGMI_FB_SHARING_MODE_8;
		break;
	default:
		fb_sharing_mode = SMI_XGMI_FB_SHARING_MODE_UNKNOWN;
		break;
	}

	return fb_sharing_mode;
}

enum amdgv_memory_partition_mode smi_map_memory_partition_mode(enum smi_memory_partition_type mode)
{
	enum amdgv_memory_partition_mode memory_mode;

	switch (mode) {
	case SMI_MEMORY_PARTITION_NPS1:
		memory_mode = AMDGV_MEMORY_PARTITION_MODE_NPS1;
		break;
	case SMI_MEMORY_PARTITION_NPS2:
		memory_mode = AMDGV_MEMORY_PARTITION_MODE_NPS2;
		break;
	case SMI_MEMORY_PARTITION_NPS4:
		memory_mode = AMDGV_MEMORY_PARTITION_MODE_NPS4;
		break;
	case SMI_MEMORY_PARTITION_NPS8:
		memory_mode = AMDGV_MEMORY_PARTITION_MODE_NPS8;
		break;
	default:
		memory_mode = AMDGV_MEMORY_PARTITION_MODE_UNKNOWN;
		break;
	}

	return memory_mode;
}

enum smi_link_status smi_map_link_status(enum amdgv_gpumon_link_status amdgv_link_status)
{
	uint32_t link_status;

	switch (amdgv_link_status) {
	case AMDGV_GPUMON_LINK_STATUS_ENABLED:
		link_status = SMI_LINK_STATUS_ENABLED;
		break;
	case AMDGV_GPUMON_LINK_STATUS_DISABLED:
		link_status = SMI_LINK_STATUS_DISABLED;
		break;
	case AMDGV_GPUMON_LINK_STATUS_INACTIVE:
		link_status = SMI_LINK_STATUS_INACTIVE;
		break;
	default:
		link_status = SMI_LINK_STATUS_ERROR;
		break;
	}

	return link_status;
}

enum smi_npm_status smi_map_npm_status(enum AMDGV_GPU_NPM_STATUS amdgv_npm_status)
{
	int npm_status;

	switch (amdgv_npm_status) {
	case AMDGPUMON_NPM_DISABLED:
		npm_status = SMI_NPM_STATUS_DISABLED;
		break;
	case AMDGPUMON_NPM_ENABLED:
		npm_status = SMI_NPM_STATUS_ENABLED;
		break;
	default:
		npm_status = 0xFFFFFFFF;
		break;
	}

	return npm_status;
}

enum smi_link_type smi_map_link_type(enum amdgv_gpumon_link_type amdgv_link_type)
{
	uint32_t link_type;

	switch (amdgv_link_type) {
	case AMDGV_GPUMON_LINK_TYPE_PCIE:
		link_type = SMI_LINK_TYPE_PCIE;
		break;
	case AMDGV_GPUMON_LINK_TYPE_XGMI3:
		link_type = SMI_LINK_TYPE_XGMI;
		break;
	case AMDGV_GPUMON_LINK_TYPE_NOT_APPLICABLE:
		link_type = SMI_LINK_TYPE_NOT_APPLICABLE;
		break;
	default:
		link_type = SMI_LINK_TYPE_UNKNOWN;
		break;
	}

	return link_type;
}

enum smi_card_form_factor smi_map_card_form_factor(enum amdgv_gpumon_card_form_factor type)
{
	uint32_t card_type;

	switch (type) {
	case AMDGV_GPUMON_CARD_FORM_FACTOR__PCIE:
		card_type = SMI_CARD_FORM_FACTOR_PCIE;
		break;
	case AMDGV_GPUMON_CARD_FORM_FACTOR__OAM:
		card_type = SMI_CARD_FORM_FACTOR_OAM;
		break;
	default:
		card_type = SMI_CARD_FORM_FACTOR_UNKNOWN;
		break;
	}

	return card_type;
}

enum amdgv_smi_ras_block smi_map_gpu_block(enum smi_gpu_block block)
{
	uint32_t gpu_block;

	switch (block) {
	case SMI_GPU_BLOCK_UMC:
		gpu_block = AMDGV_SMI_RAS_BLOCK__UMC;
		break;
	case SMI_GPU_BLOCK_SDMA:
		gpu_block = AMDGV_SMI_RAS_BLOCK__SDMA;
		break;
	case SMI_GPU_BLOCK_GFX:
		gpu_block = AMDGV_SMI_RAS_BLOCK__GFX;
		break;
	case SMI_GPU_BLOCK_MMHUB:
		gpu_block = AMDGV_SMI_RAS_BLOCK__MMHUB;
		break;
	case SMI_GPU_BLOCK_ATHUB:
		gpu_block = AMDGV_SMI_RAS_BLOCK__ATHUB;
		break;
	case SMI_GPU_BLOCK_PCIE_BIF:
		gpu_block = AMDGV_SMI_RAS_BLOCK__PCIE_BIF;
		break;
	case SMI_GPU_BLOCK_HDP:
		gpu_block = AMDGV_SMI_RAS_BLOCK__HDP;
		break;
	case SMI_GPU_BLOCK_XGMI_WAFL:
		gpu_block = AMDGV_SMI_RAS_BLOCK__XGMI_WAFL;
		break;
	case SMI_GPU_BLOCK_DF:
		gpu_block = AMDGV_SMI_RAS_BLOCK__DF;
		break;
	case SMI_GPU_BLOCK_SMN:
		gpu_block = AMDGV_SMI_RAS_BLOCK__SMN;
		break;
	case SMI_GPU_BLOCK_SEM:
		gpu_block = AMDGV_SMI_RAS_BLOCK__SEM;
		break;
	case SMI_GPU_BLOCK_MP0:
		gpu_block = AMDGV_SMI_RAS_BLOCK__MP0;
		break;
	case SMI_GPU_BLOCK_MP1:
		gpu_block = AMDGV_SMI_RAS_BLOCK__MP1;
		break;
	case SMI_GPU_BLOCK_FUSE:
		gpu_block = AMDGV_SMI_RAS_BLOCK__FUSE;
		break;
	case SMI_GPU_BLOCK_MCA:
		gpu_block = AMDGV_SMI_RAS_BLOCK__MCA;
		break;
	case SMI_GPU_BLOCK_VCN:
		gpu_block = AMDGV_SMI_RAS_BLOCK__VCN;
		break;
	case SMI_GPU_BLOCK_JPEG:
		gpu_block = AMDGV_SMI_RAS_BLOCK__JPEG;
		break;
	case SMI_GPU_BLOCK_IH:
		gpu_block = AMDGV_SMI_RAS_BLOCK__IH;
		break;
	case SMI_GPU_BLOCK_MPIO:
		gpu_block = AMDGV_SMI_RAS_BLOCK__MPIO;
		break;
	default:
		gpu_block = AMDGV_SMI_NUM_BLOCK_MAX;
		break;
	}

	return gpu_block;
}

enum smi_vram_type smi_map_vram_type(enum amdgv_gpumon_vram_type type)
{
	uint32_t vram_type;

	switch (type) {
	case AMDGV_GPUMON_DGPU_VRAM_TYPE__HBM2:
		vram_type = SMI_VRAM_TYPE_HBM2;
		break;
	case AMDGV_GPUMON_DGPU_VRAM_TYPE__HBM2E:
		vram_type = SMI_VRAM_TYPE_HBM2E;
		break;
	case AMDGV_GPUMON_DGPU_VRAM_TYPE__HBM3:
		vram_type = SMI_VRAM_TYPE_HBM3;
		break;
	case AMDGV_GPUMON_DGPU_VRAM_TYPE__HBM3E:
		vram_type = SMI_VRAM_TYPE_HBM3E;
		break;
	case AMDGV_GPUMON_DGPU_VRAM_TYPE__GDDR5:
		vram_type = SMI_VRAM_TYPE_GDDR5;
		break;
	case AMDGV_GPUMON_DGPU_VRAM_TYPE__GDDR6:
		vram_type = SMI_VRAM_TYPE_GDDR6;
		break;
	case AMDGV_GPUMON_DGPU_VRAM_TYPE__GDDR7:
		vram_type = SMI_VRAM_TYPE_GDDR7;
		break;
	default:
		vram_type = SMI_VRAM_TYPE_UNKNOWN;
		break;
	}

	return vram_type;
}

const char* smi_map_vram_vendor(enum amdgv_gpumon_vram_vendor vendor)
{
	switch (vendor) {
	case AMDGV_GPUMON_VRAM_VENDOR__SAMSUNG:
		return "SAMSUNG";
	case AMDGV_GPUMON_VRAM_VENDOR__INFINEON:
		return "INFINEON";
	case AMDGV_GPUMON_VRAM_VENDOR__ELPIDA:
		return "ELPIDA";
	case AMDGV_GPUMON_VRAM_VENDOR__ETRON:
		return "ETRON";
	case AMDGV_GPUMON_VRAM_VENDOR__NANYA:
		return "NANYA";
	case AMDGV_GPUMON_VRAM_VENDOR__HYNIX:
		return "HYNIX";
	case AMDGV_GPUMON_VRAM_VENDOR__MOSEL:
		return "MOSEL";
	case AMDGV_GPUMON_VRAM_VENDOR__WINBOND:
		return "WINBOND";
	case AMDGV_GPUMON_VRAM_VENDOR__ESMT:
		return "ESMT";
	case AMDGV_GPUMON_VRAM_VENDOR__MICRON:
		return "MICRON";
	default:
		break;
	}

	return "UNKNOWN";
}

int smi_compare_dev_bdf(const void *a, const void *b)
{
	struct smi_device_data *tmp_a;
	struct smi_device_data *tmp_b;

	tmp_a = (struct smi_device_data *)a;
	tmp_b = (struct smi_device_data *)b;
	return (tmp_a->init_data.info.bdf - tmp_b->init_data.info.bdf);
}

enum smi_metric_category smi_map_metric_category(enum amdgv_gpumon_metric_ext_category category)
{
	enum smi_metric_category metric_category = SMI_METRIC_CATEGORY_UNKNOWN;

	switch (category) {
	case AMDGV_GPUMON_METRIC_EXT_CATEGORY__ACC_COUNTER:
		metric_category = SMI_METRIC_CATEGORY_ACC_COUNTER;
		break;
	case AMDGV_GPUMON_METRIC_EXT_CATEGORY__FREQUENCY:
		metric_category = SMI_METRIC_CATEGORY_FREQUENCY;
		break;
	case AMDGV_GPUMON_METRIC_EXT_CATEGORY__ACTIVITY:
		metric_category = SMI_METRIC_CATEGORY_ACTIVITY;
		break;
	case AMDGV_GPUMON_METRIC_EXT_CATEGORY__TEMPERATURE:
		metric_category = SMI_METRIC_CATEGORY_TEMPERATURE;
		break;
	case AMDGV_GPUMON_METRIC_EXT_CATEGORY__POWER:
		metric_category = SMI_METRIC_CATEGORY_POWER;
		break;
	case AMDGV_GPUMON_METRIC_EXT_CATEGORY__ENERGY:
		metric_category = SMI_METRIC_CATEGORY_ENERGY;
		break;
	case AMDGV_GPUMON_METRIC_EXT_CATEGORY__THROTTLE:
		metric_category = SMI_METRIC_CATEGORY_THROTTLE;
		break;
	case AMDGV_GPUMON_METRIC_EXT_CATEGORY__PCIE:
		metric_category = SMI_METRIC_CATEGORY_PCIE;
		break;
	case AMDGV_GPUMON_METRIC_EXT_CATEGORY__STATIC:
		metric_category = SMI_METRIC_CATEGORY_STATIC;
		break;
	case AMDGV_GPUMON_METRIC_EXT_CATEGORY__SYS_ACC_COUNTER:
		metric_category = SMI_METRIC_CATEGORY_SYS_ACC_COUNTER;
		break;
	case AMDGV_GPUMON_METRIC_EXT_CATEGORY__SYS_BASEBOARD_TEMP:
		metric_category = SMI_METRIC_CATEGORY_SYS_BASEBOARD_TEMP;
		break;
	case AMDGV_GPUMON_METRIC_EXT_CATEGORY__SYS_GPUBOARD_TEMP:
		metric_category = SMI_METRIC_CATEGORY_SYS_GPUBOARD_TEMP;
		break;
	default:
		metric_category = SMI_METRIC_CATEGORY_UNKNOWN;
		break;
	}

	return metric_category;
}

enum smi_metric_name smi_map_metric_name(enum amdgv_gpumon_metric_ext_name name)
{
	enum smi_metric_name metric_name = SMI_METRIC_NAME_UNKNOWN;

	switch (name) {
	case AMDGV_GPUMON_METRIC_EXT_NAME__METRIC_ACC_COUNTER:
		metric_name = SMI_METRIC_NAME_METRIC_ACC_COUNTER;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__FW_TIMESTAMP:
		metric_name = SMI_METRIC_NAME_FW_TIMESTAMP;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_GFX:
		metric_name = SMI_METRIC_NAME_CLK_GFX;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_SOC:
		metric_name = SMI_METRIC_NAME_CLK_SOC;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_MEM:
		metric_name = SMI_METRIC_NAME_CLK_MEM;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_VCLK:
		metric_name = SMI_METRIC_NAME_CLK_VCLK;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_DCLK:
		metric_name = SMI_METRIC_NAME_CLK_DCLK;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__USAGE_GFX:
		metric_name = SMI_METRIC_NAME_USAGE_GFX;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__USAGE_MEM:
		metric_name = SMI_METRIC_NAME_USAGE_MEM;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__USAGE_MM:
		metric_name = SMI_METRIC_NAME_USAGE_MM;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__USAGE_VCN:
		metric_name = SMI_METRIC_NAME_USAGE_VCN;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__USAGE_JPEG:
		metric_name = SMI_METRIC_NAME_USAGE_JPEG;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__VOLT_GFX:
		metric_name = SMI_METRIC_NAME_VOLT_GFX;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__VOLT_SOC:
		metric_name = SMI_METRIC_NAME_VOLT_SOC;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__VOLT_MEM:
		metric_name = SMI_METRIC_NAME_VOLT_MEM;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_HOTSPOT_CURR:
		metric_name = SMI_METRIC_NAME_TEMP_HOTSPOT_CURR;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_HOTSPOT_LIMIT:
		metric_name = SMI_METRIC_NAME_TEMP_HOTSPOT_LIMIT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_MEM_CURR:
		metric_name = SMI_METRIC_NAME_TEMP_MEM_CURR;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_MEM_LIMIT:
		metric_name = SMI_METRIC_NAME_TEMP_MEM_LIMIT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_VR_CURR:
		metric_name = SMI_METRIC_NAME_TEMP_VR_CURR;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_SHUTDOWN:
		metric_name = SMI_METRIC_NAME_TEMP_SHUTDOWN;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__POWER_CURR:
		metric_name = SMI_METRIC_NAME_POWER_CURR;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__POWER_LIMIT:
		metric_name = SMI_METRIC_NAME_POWER_LIMIT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__ENERGY_SOCKET:
		metric_name = SMI_METRIC_NAME_ENERGY_SOCKET;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__ENERGY_CCD:
		metric_name = SMI_METRIC_NAME_ENERGY_CCD;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__ENERGY_XCD:
		metric_name = SMI_METRIC_NAME_ENERGY_XCD;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__ENERGY_AID:
		metric_name = SMI_METRIC_NAME_ENERGY_AID;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__ENERGY_MEM:
		metric_name = SMI_METRIC_NAME_ENERGY_MEM;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__THROTTLE_SOCKET_ACTIVE:
		metric_name = SMI_METRIC_NAME_THROTTLE_SOCKET_ACTIVE;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__THROTTLE_VR_ACTIVE:
		metric_name = SMI_METRIC_NAME_THROTTLE_VR_ACTIVE;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__THROTTLE_MEM_ACTIVE:
		metric_name = SMI_METRIC_NAME_THROTTLE_MEM_ACTIVE;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__PCIE_BANDWIDTH:
		metric_name = SMI_METRIC_NAME_PCIE_BANDWIDTH;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__PCIE_L0_TO_RECOVERY_COUNT:
		metric_name = SMI_METRIC_NAME_PCIE_L0_TO_RECOVERY_COUNT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__PCIE_REPLAY_COUNT:
		metric_name = SMI_METRIC_NAME_PCIE_REPLAY_COUNT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__PCIE_REPLAY_ROLLOVER_COUNT:
		metric_name = SMI_METRIC_NAME_PCIE_REPLAY_ROLLOVER_COUNT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__PCIE_NAK_SENT_COUNT:
		metric_name = SMI_METRIC_NAME_PCIE_NAK_SENT_COUNT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__PCIE_NAK_RECEIVED_COUNT:
		metric_name = SMI_METRIC_NAME_PCIE_NAK_RECEIVED_COUNT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_GFX_MAX_LIMIT:
		metric_name = SMI_METRIC_NAME_CLK_GFX_MAX_LIMIT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_SOC_MAX_LIMIT:
		metric_name = SMI_METRIC_NAME_CLK_SOC_MAX_LIMIT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_MEM_MAX_LIMIT:
		metric_name = SMI_METRIC_NAME_CLK_MEM_MAX_LIMIT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_VCLK_MAX_LIMIT:
		metric_name = SMI_METRIC_NAME_CLK_VCLK_MAX_LIMIT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_DCLK_MAX_LIMIT:
		metric_name = SMI_METRIC_NAME_CLK_DCLK_MAX_LIMIT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_GFX_MIN_LIMIT:
		metric_name = SMI_METRIC_NAME_CLK_GFX_MIN_LIMIT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_SOC_MIN_LIMIT:
		metric_name = SMI_METRIC_NAME_CLK_SOC_MIN_LIMIT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_MEM_MIN_LIMIT:
		metric_name = SMI_METRIC_NAME_CLK_MEM_MIN_LIMIT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_VCLK_MIN_LIMIT:
		metric_name = SMI_METRIC_NAME_CLK_VCLK_MIN_LIMIT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_DCLK_MIN_LIMIT:
		metric_name = SMI_METRIC_NAME_CLK_DCLK_MIN_LIMIT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_GFX_LOCKED:
		metric_name = SMI_METRIC_NAME_CLK_GFX_LOCKED;
		break;
	/*case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_GFX_DS_DISABLED:
		metric_name = SMI_METRIC_NAME_CLK_GFX_DS_DISABLED;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_MEM_DS_DISABLED:
		metric_name = SMI_METRIC_NAME_CLK_MEM_DS_DISABLED;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_SOC_DS_DISABLED:
		metric_name = SMI_METRIC_NAME_CLK_SOC_DS_DISABLED;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_VCLK_DS_DISABLED:
		metric_name = SMI_METRIC_NAME_CLK_VCLK_DS_DISABLED;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__CLK_DCLK_DS_DISABLED:
		metric_name = SMI_METRIC_NAME_CLK_DCLK_DS_DISABLED;
		break;*/
	case AMDGV_GPUMON_METRIC_EXT_NAME__PCIE_LINK_SPEED:
		metric_name = SMI_METRIC_NAME_PCIE_LINK_SPEED;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__PCIE_LINK_WIDTH:
		metric_name = SMI_METRIC_NAME_PCIE_LINK_WIDTH;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__DRAM_BANDWIDTH:
		metric_name = SMI_METRIC_NAME_DRAM_BANDWIDTH;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__MAX_DRAM_BANDWIDTH:
		metric_name = SMI_METRIC_NAME_MAX_DRAM_BANDWIDTH;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__GFX_CLK_BELOW_HOST_LIMIT_PPT:
		metric_name = SMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_PPT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__GFX_CLK_BELOW_HOST_LIMIT_THM:
		metric_name = SMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_THM;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__GFX_CLK_BELOW_HOST_LIMIT_TOTAL:
		metric_name = SMI_METRIC_NAME_GFX_CLK_BELOW_HOST_LIMIT_TOTAL;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__GFX_CLK_LOW_UTILIZATION:
		metric_name = SMI_METRIC_NAME_GFX_CLK_LOW_UTILIZATION;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__INPUT_TELEMETRY_VOLTAGE:
		metric_name = SMI_METRIC_NAME_INPUT_TELEMETRY_VOLTAGE;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__PLDM_VERSION:
		metric_name = SMI_METRIC_NAME_PLDM_VERSION;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_XCD:
		metric_name = SMI_METRIC_NAME_TEMP_XCD;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_AID:
		metric_name = SMI_METRIC_NAME_TEMP_AID;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__TEMP_HBM:
		metric_name = SMI_METRIC_NAME_TEMP_HBM;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYS_METRIC_ACC_COUNTER:
		metric_name = SMI_METRIC_NAME_SYS_METRIC_ACC_COUNTER;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_UBB_FPGA:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_UBB_FPGA;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_UBB_FRONT:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_UBB_FRONT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_UBB_BACK:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_UBB_BACK;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_UBB_OAM7:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_UBB_OAM7;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_UBB_IBC:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_UBB_IBC;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_UBB_UFPGA:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_UBB_UFPGA;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_UBB_OAM1:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_UBB_OAM1;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_OAM_0_1_HSC:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_OAM_0_1_HSC;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_OAM_2_3_HSC:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_OAM_2_3_HSC;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_OAM_4_5_HSC:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_OAM_4_5_HSC;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_OAM_6_7_HSC:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_OAM_6_7_HSC;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_UBB_FPGA_0V72_VR:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_UBB_FPGA_0V72_VR;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_UBB_FPGA_3V3_VR:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_UBB_FPGA_3V3_VR;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_RETIMER_0_1_2_3_1V2_VR:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_0_1_2_3_1V2_VR;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_RETIMER_4_5_6_7_1V2_VR:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_4_5_6_7_1V2_VR;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_RETIMER_0_1_0V9_VR:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_0_1_0V9_VR;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_RETIMER_4_5_0V9_VR:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_4_5_0V9_VR;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_RETIMER_2_3_0V9_VR:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_2_3_0V9_VR;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_RETIMER_6_7_0V9_VR:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_RETIMER_6_7_0V9_VR;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_OAM_0_1_2_3_3V3_VR:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_OAM_0_1_2_3_3V3_VR;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_OAM_4_5_6_7_3V3_VR:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_OAM_4_5_6_7_3V3_VR;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_IBC_HSC:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_IBC_HSC;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__SYSTEM_TEMP_IBC:
		metric_name = SMI_METRIC_NAME_SYSTEM_TEMP_IBC;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__NODE_TEMP_RETIMER:
		metric_name = SMI_METRIC_NAME_NODE_TEMP_RETIMER;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__NODE_TEMP_IBC_TEMP:
		metric_name = SMI_METRIC_NAME_NODE_TEMP_IBC_TEMP;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__NODE_TEMP_IBC_2_TEMP:
		metric_name = SMI_METRIC_NAME_NODE_TEMP_IBC_2_TEMP;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__NODE_TEMP_VDD18_VR_TEMP:
		metric_name = SMI_METRIC_NAME_NODE_TEMP_VDD18_VR_TEMP;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__NODE_TEMP_04_HBM_B_VR_TEMP:
		metric_name = SMI_METRIC_NAME_NODE_TEMP_04_HBM_B_VR_TEMP;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__NODE_TEMP_04_HBM_D_VR_TEMP:
		metric_name = SMI_METRIC_NAME_NODE_TEMP_04_HBM_D_VR_TEMP;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__VR_TEMP_VDDCR_VDD0:
		metric_name = SMI_METRIC_NAME_VR_TEMP_VDDCR_VDD0;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__VR_TEMP_VDDCR_VDD1:
		metric_name = SMI_METRIC_NAME_VR_TEMP_VDDCR_VDD1;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__VR_TEMP_VDDCR_VDD2:
		metric_name = SMI_METRIC_NAME_VR_TEMP_VDDCR_VDD2;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__VR_TEMP_VDDCR_VDD3:
		metric_name = SMI_METRIC_NAME_VR_TEMP_VDDCR_VDD3;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__VR_TEMP_VDDCR_SOC_A:
		metric_name = SMI_METRIC_NAME_VR_TEMP_VDDCR_SOC_A;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__VR_TEMP_VDDCR_SOC_C:
		metric_name = SMI_METRIC_NAME_VR_TEMP_VDDCR_SOC_C;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__VR_TEMP_VDDCR_SOCIO_A:
		metric_name = SMI_METRIC_NAME_VR_TEMP_VDDCR_SOCIO_A;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__VR_TEMP_VDDCR_SOCIO_C:
		metric_name = SMI_METRIC_NAME_VR_TEMP_VDDCR_SOCIO_C;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__VR_TEMP_VDD_085_HBM:
		metric_name = SMI_METRIC_NAME_VR_TEMP_VDD_085_HBM;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__VR_TEMP_VDDCR_11_HBM_B:
		metric_name = SMI_METRIC_NAME_VR_TEMP_VDDCR_11_HBM_B;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__VR_TEMP_VDDCR_11_HBM_D:
		metric_name = SMI_METRIC_NAME_VR_TEMP_VDDCR_11_HBM_D;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__VR_TEMP_VDD_USR:
		metric_name = SMI_METRIC_NAME_VR_TEMP_VDD_USR;
		break;
	case AMDGV_GPUMON_METRIC_EXT_NAME__VR_TEMP_VDDIO_11_E32:
		metric_name = SMI_METRIC_NAME_VR_TEMP_VDDIO_11_E32;
		break;
	default:
		metric_name = SMI_METRIC_NAME_UNKNOWN;
		break;
	}

	return metric_name;
}

enum smi_metric_unit smi_map_metric_unit(enum amdgv_gpumon_metric_ext_unit unit)
{
	enum smi_metric_unit metric_unit = SMI_METRIC_UNIT_UNKNOWN;

	switch (unit) {
	case AMDGV_GPUMON_METRIC_EXT_UNIT__COUNTER:
		metric_unit = SMI_METRIC_UNIT_COUNTER;
		break;
	case AMDGV_GPUMON_METRIC_EXT_UNIT__UINT:
		metric_unit = SMI_METRIC_UNIT_UINT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_UNIT__BOOL:
		metric_unit = SMI_METRIC_UNIT_BOOL;
		break;
	case AMDGV_GPUMON_METRIC_EXT_UNIT__MHZ:
		metric_unit = SMI_METRIC_UNIT_MHZ;
		break;
	case AMDGV_GPUMON_METRIC_EXT_UNIT__PERCENT:
		metric_unit = SMI_METRIC_UNIT_PERCENT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_UNIT__MILLIVOLT:
		metric_unit = SMI_METRIC_UNIT_MILLIVOLT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_UNIT__CELSIUS:
		metric_unit = SMI_METRIC_UNIT_CELSIUS;
		break;
	case AMDGV_GPUMON_METRIC_EXT_UNIT__WATT:
		metric_unit = SMI_METRIC_UNIT_WATT;
		break;
	case AMDGV_GPUMON_METRIC_EXT_UNIT__JOULE:
		metric_unit = SMI_METRIC_UNIT_JOULE;
		break;
	case AMDGV_GPUMON_METRIC_EXT_UNIT__GBPS:
		metric_unit = SMI_METRIC_UNIT_GBPS;
		break;
	case AMDGV_GPUMON_METRIC_EXT_UNIT__MBITPS:
		metric_unit = SMI_METRIC_UNIT_MBITPS;
		break;
	case AMDGV_GPUMON_METRIC_EXT_UNIT__PCIE_GEN:
		metric_unit = SMI_METRIC_UNIT_PCIE_GEN;
		break;
	case AMDGV_GPUMON_METRIC_EXT_UNIT__PCIE_LANES:
		metric_unit = SMI_METRIC_UNIT_PCIE_LANES;
		break;
	case AMDGV_GPUMON_METRIC_EXT_UNIT__15_625_MILLIJOULE:
		metric_unit = SMI_METRIC_UNIT__15_625_MILLIJOULE;
		break;
	default:
		metric_unit = SMI_METRIC_UNIT_UNKNOWN;
	}

	return metric_unit;
}

enum smi_metric_res_group smi_map_metric_res_group(enum amdgv_gpumon_metric_ext_res_group res_group)
{
	enum smi_metric_res_group metric_res_group = SMI_METRIC_RES_GROUP_UNKNOWN;

	switch (res_group) {
	case AMDGV_GPUMON_METRIC_EXT_RES_GROUP__NA:
		metric_res_group = SMI_METRIC_RES_GROUP_NA;
		break;
	case AMDGV_GPUMON_METRIC_EXT_RES_GROUP__GPU:
		metric_res_group = SMI_METRIC_RES_GROUP_GPU;
		break;
	case AMDGV_GPUMON_METRIC_EXT_RES_GROUP__XCP:
		metric_res_group = SMI_METRIC_RES_GROUP_XCP;
		break;
	case AMDGV_GPUMON_METRIC_EXT_RES_GROUP__AID:
		metric_res_group = SMI_METRIC_RES_GROUP_AID;
		break;
	case AMDGV_GPUMON_METRIC_EXT_RES_GROUP__MID:
		metric_res_group = SMI_METRIC_RES_GROUP_MID;
		break;
	case AMDGV_GPUMON_METRIC_EXT_RES_GROUP__SYSTEM:
		metric_res_group = SMI_METRIC_RES_GROUP_SYSTEM;
		break;
	default:
		metric_res_group = SMI_METRIC_RES_GROUP_UNKNOWN;
	}

	return metric_res_group;
}

enum smi_metric_res_subgroup smi_map_metric_res_subgroup(enum amdgv_gpumon_metric_ext_res_subgroup res_subgroup)
{
	enum smi_metric_res_group metric_res_subgroup = SMI_METRIC_RES_SUBGROUP_UNKNOWN;

	switch (res_subgroup) {
	case AMDGV_GPUMON_METRIC_EXT_RES_SUBGROUP__NA:
		metric_res_subgroup = SMI_METRIC_RES_SUBGROUP_NA;
		break;
	case AMDGV_GPUMON_METRIC_EXT_RES_SUBGROUP__XCC:
		metric_res_subgroup = SMI_METRIC_RES_SUBGROUP_XCC;
		break;
	case AMDGV_GPUMON_METRIC_EXT_RES_SUBGROUP__ENGINE:
		metric_res_subgroup = SMI_METRIC_RES_SUBGROUP_ENGINE;
		break;
	case AMDGV_GPUMON_METRIC_EXT_RES_SUBGROUP__HBM:
		metric_res_subgroup = SMI_METRIC_RES_SUBGROUP_HBM;
		break;
	case AMDGV_GPUMON_METRIC_EXT_RES_SUBGROUP__BASEBOARD:
		metric_res_subgroup = SMI_METRIC_RES_SUBGROUP_BASEBOARD;
		break;
	case AMDGV_GPUMON_METRIC_EXT_RES_SUBGROUP__GPUBOARD:
		metric_res_subgroup = SMI_METRIC_RES_SUBGROUP_GPUBOARD;
		break;
	default:
		metric_res_subgroup = SMI_METRIC_RES_SUBGROUP_UNKNOWN;
	}

	return metric_res_subgroup;
}


enum smi_memory_partition_type smi_map_mp_mode(enum amdgv_memory_partition_mode mode)
{
	enum smi_memory_partition_type partition_mode = SMI_MEMORY_PARTITION_UNKNOWN;

	switch (mode) {
	case AMDGV_MEMORY_PARTITION_MODE_NPS1:
		partition_mode = SMI_MEMORY_PARTITION_NPS1;
		break;
	case AMDGV_MEMORY_PARTITION_MODE_NPS2:
		partition_mode = SMI_MEMORY_PARTITION_NPS2;
		break;
	case AMDGV_MEMORY_PARTITION_MODE_NPS4:
		partition_mode = SMI_MEMORY_PARTITION_NPS4;
		break;
	case AMDGV_MEMORY_PARTITION_MODE_NPS8:
		partition_mode = SMI_MEMORY_PARTITION_NPS8;
		break;
	default:
		partition_mode = SMI_MEMORY_PARTITION_UNKNOWN;
	}

	return partition_mode;
}

enum smi_accelerator_partition_type smi_map_partition_type(enum amdgv_gpumon_acccelerator_partition_type type)
{
	enum smi_accelerator_partition_type partition_type = SMI_ACCELERATOR_PARTITION_INVALID;

	switch (type) {
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_SPX:
		partition_type = SMI_ACCELERATOR_PARTITION_SPX;
		break;
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_DPX:
		partition_type = SMI_ACCELERATOR_PARTITION_DPX;
		break;
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_TPX:
		partition_type = SMI_ACCELERATOR_PARTITION_TPX;
		break;
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_QPX:
		partition_type = SMI_ACCELERATOR_PARTITION_QPX;
		break;
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_CPX:
		partition_type = SMI_ACCELERATOR_PARTITION_CPX;
		break;
	default:
		partition_type = SMI_ACCELERATOR_PARTITION_INVALID;
	}

	return partition_type;
}

enum smi_accelerator_partition_resource_type smi_map_resource_type(enum amdgv_gpumon_accelerator_partition_resource_type type)
{
	enum smi_accelerator_partition_resource_type resource_type = SMI_ACCELERATOR_MAX;

	switch (type) {
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_XCC:
		resource_type = SMI_ACCELERATOR_XCC;
		break;
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_ENCODER:
		resource_type = SMI_ACCELERATOR_ENCODER;
		break;
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_DECODER:
		resource_type = SMI_ACCELERATOR_DECODER;
		break;
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_DMA:
		resource_type = SMI_ACCELERATOR_DMA;
		break;
	case AMDGV_GPUMON_ACCELERATOR_PARTITION_RESOURCE_JPEG:
		resource_type = SMI_ACCELERATOR_JPEG;
		break;
	default:
		resource_type = SMI_ACCELERATOR_MAX;
	}

	return resource_type;
}
enum smi_vf_sched_state smi_map_sched_state(enum amdgv_sched_state state)
{
	switch (state) {
	case AMDGV_SCHED_UNAVAL:
		return SMI_VF_STATE_UNAVAILABLE;
	case AMDGV_SCHED_AVAIL:
		return SMI_VF_STATE_AVAILABLE;
	case AMDGV_SCHED_ACTIVE:
		return SMI_VF_STATE_ACTIVE;
	case AMDGV_SCHED_SUSPEND:
		return SMI_VF_STATE_SUSPENDED;
	case AMDGV_SCHED_FULLACCESS:
		return SMI_VF_STATE_FULLACCESS;
	default:
		return SMI_VF_STATE_UNAVAILABLE;
	}
}
