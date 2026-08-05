/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef SMU_V15_0_8_INTERNAL_H
#define SMU_V15_0_8_INTERNAL_H

#include "smu_v15_0_8_ppsmc.h"
#include "smu_v15_0_8_driver_if.h"
#include "smu_v15_0_8_pmfw.h"

#define MP1_Public  0x03b00000

#define PAGE_SIZE 0x1000
#define TOOL_SIZE 0x19000

#define SMUQ10_TO_UINT(x) ((x) >> 10)
#define SMUQ16_TO_UINT(x) ((x) >> 16)
#define SMUQ10_FRAC(x) ((x) & 0x3ff)
#define SMUQ10_ROUND(x) ((SMUQ10_TO_UINT(x)) + ((SMUQ10_FRAC(x)) >= 0x200))

#define PLDM_VERSION_NOT_SUPPORTED	0xffffffff
#define SMU_15_0_8_MAX_ARGS		4

struct smu_15_0_8_msg {
	uint32_t id;
	uint32_t in_arg[SMU_15_0_8_MAX_ARGS];
	uint32_t out_arg[SMU_15_0_8_MAX_ARGS];
};

// This DS threshold was calculated based on GPU characteristics using the formula
// max GFXCLK/GFX DS clock divider =  (2250/16) ~= 140
#define AMDGV_GPUMON_DS_THRESHOLD 140

int smu_v15_0_8_send_msg(struct amdgv_adapter *adapt, struct smu_15_0_8_msg *msg);
void smu_v15_0_8_ack_irq(struct amdgv_adapter *adapt);
bool smu_v15_0_8_is_fw_alive(struct amdgv_adapter *adapt);

#endif
