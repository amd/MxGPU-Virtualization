/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef __AMDGV_NPS_H__
#define __AMDGV_NPS_H__

#include <amdgv.h>
#include <amdgv_device.h>

#define AMDGV_VF_NPS_MAX_COMBINATIONS		4
#define AMDGV_NPS_COMPUTE_MAX_COMBINATIONS	12

struct amdgv_nps_compute_combination {
	enum amdgv_memory_partition_mode nps_mode;
	enum amdgv_accelerator_partition_mode compute_mode;
};

struct amdgv_vf_nps_combination {
	uint32_t vf_num;
	struct amdgv_nps_compute_combination combinations[AMDGV_NPS_COMPUTE_MAX_COMBINATIONS];
};

struct amdgv_nps_combination_cap_entry {
	enum amd_asic_type asic_type;
	struct amdgv_vf_nps_combination vf_nps[AMDGV_VF_NPS_MAX_COMBINATIONS];
};

#endif
