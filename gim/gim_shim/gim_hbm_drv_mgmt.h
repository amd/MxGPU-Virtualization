/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <linux/types.h>

int gim_hbm_drv_mgmt_module_init(void);
void gim_hbm_drv_mgmt_module_fini(void);

/* pf_bdf is the PF BDF. idx_vf selects the SRAT handle at PF devfn + idx_vf + 1.
 * *numa_id receives the Linux node.
 * *phy_addr / *phy_size: input request; on success, updated to aligned region.
 */
void *gim_hbm_drv_mgmt_init(const char *hbm_name, uint32_t pf_bdf,
			    uint32_t idx_vf, int *numa_id,
			    uint64_t *phy_addr, uint64_t *phy_size);

void gim_hbm_drv_mgmt_fini(void *hbm_drv_mgmt);
