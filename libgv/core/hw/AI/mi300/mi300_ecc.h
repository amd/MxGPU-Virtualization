/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#ifndef AMDGV_MI300_ECC_H
#define AMDGV_MI300_ECC_H

#define AMDGV_RAS_POISON_PROPOGATION_MODE_BIT (1 << 24)
#define AMDGV_RAS_MAX_BAD_GPU_CHECK_RETRY 100

extern struct amdgv_init_func mi300_ecc_func;
uint32_t mi300_get_ras_cap(struct amdgv_adapter *adapt);
void mi300_ras_query_boot_status(struct amdgv_adapter *adapt, uint32_t num_instances);

struct mi300_ras_cap_entry {
	uint32_t dev_id;
	uint32_t ras_cap;
};

#endif
