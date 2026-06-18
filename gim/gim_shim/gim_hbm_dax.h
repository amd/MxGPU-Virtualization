/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <linux/types.h>

void *gim_hbm_dax_init(const char *dev_name,
			uint64_t phy_addr, uint64_t phy_size);

void gim_hbm_dax_fini(void *dax);
