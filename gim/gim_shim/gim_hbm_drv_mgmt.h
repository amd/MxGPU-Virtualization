/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <linux/types.h>

int gim_hbm_drv_mgmt_module_init(void);
void gim_hbm_drv_mgmt_module_fini(void);

/* *phy_addr / *phy_size: input request; on success, updated to aligned region */
void *gim_hbm_drv_mgmt_init(const char *hbm_name, int numa_id,
			    uint64_t *phy_addr, uint64_t *phy_size);

void gim_hbm_drv_mgmt_fini(void *hbm_drv_mgmt);
