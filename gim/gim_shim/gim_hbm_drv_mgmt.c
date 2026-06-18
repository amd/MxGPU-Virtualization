/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/cdev.h>
#include <linux/io.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/memory.h>
#include <linux/errno.h>
#include "gim_debug.h"
#include "gim_hbm_drv_mgmt.h"

#define GIM_HBM_DRV_MGMT_NAME_LEN 32
struct gim_hbm_drv_mgmt_t {
	char name[GIM_HBM_DRV_MGMT_NAME_LEN];
	int numa_id;
	uint64_t phy_addr;
	uint64_t phy_size;
};

void *gim_hbm_drv_mgmt_init(const char *name, int numa_id,
					uint64_t *phy_addr, uint64_t *phy_size)
{
	int ret;
	struct gim_hbm_drv_mgmt_t *gim_hbm_drv_mgmt;
	uint64_t block_size;
	uint64_t req_addr;
	uint64_t req_size;
	uint64_t aligned_addr;
	uint64_t trim;
	uint64_t rem;
	uint64_t aligned_size;

	if (name == NULL || phy_addr == NULL || phy_size == NULL) {
		gim_warn("invalid params (name or addr/size pointers)\n");
		return NULL;
	}

	req_addr = *phy_addr;
	req_size = *phy_size;
	if (req_size == 0) {
		gim_warn("invalid params (size)\n");
		return NULL;
	}

	block_size = memory_block_size_bytes();
	if (block_size == 0 || (block_size & (block_size - 1)) != 0) {
		gim_warn("%s: invalid memory block size 0x%llx\n", name, block_size);
		return NULL;
	}

	aligned_addr = ALIGN(req_addr, block_size);
	trim = aligned_addr - req_addr;
	if (trim >= req_size) {
		gim_warn("%s: phy_addr 0x%llx align-up to 0x%llx exceeds range (phy_size 0x%llx, block 0x%llx)\n",
			name, req_addr, aligned_addr, req_size, block_size);
		return NULL;
	}
	rem = req_size - trim;
	aligned_size = ALIGN_DOWN(rem, block_size);
	if (aligned_size == 0) {
		gim_warn("%s: after align-up start=0x%llx no full block fits (rem 0x%llx, block 0x%llx)\n",
			name, aligned_addr, rem, block_size);
		return NULL;
	}

	gim_info("%s: HBM memory region aligned start=0x%llx size=0x%llx (requested 0x%llx/0x%llx) block_size=0x%llx\n",
		name, aligned_addr, aligned_size, req_addr, req_size, block_size);

	/* Allocate gim hbm mgmt context */
	gim_hbm_drv_mgmt = kzalloc(sizeof(struct gim_hbm_drv_mgmt_t), GFP_KERNEL);
	if (gim_hbm_drv_mgmt == NULL) {
		gim_warn("failed to allocate memory for gim hbm mgmt\n");
		return NULL;
	}

	gim_hbm_drv_mgmt->numa_id = numa_id;
	gim_hbm_drv_mgmt->phy_addr = aligned_addr;
	gim_hbm_drv_mgmt->phy_size = aligned_size;
	snprintf(gim_hbm_drv_mgmt->name, sizeof(gim_hbm_drv_mgmt->name),
		"System RAM (%s)", name);

	/* Add memory to NUMA node as driver-managed */
	ret = add_memory_driver_managed(
			gim_hbm_drv_mgmt->numa_id,
			gim_hbm_drv_mgmt->phy_addr,
			gim_hbm_drv_mgmt->phy_size,
			gim_hbm_drv_mgmt->name,
			MHP_NONE);
	if (ret) {
		gim_warn("%s: Failed to add HBM memory to kernel memory management (error: %d)\n",
			name, ret);
		if (ret == -EEXIST) {
			gim_warn("%s: Memory region already exists or overlaps with existing memory\n",
				name);
		} else if (ret == -EINVAL) {
			gim_warn("%s: Invalid memory region params (check alignment and address range)\n",
				name);
		} else if (ret == -ENOMEM) {
			gim_warn("%s: Insufficient memory for memory management structures\n",
				name);
		}
		kfree(gim_hbm_drv_mgmt);
		return NULL;
	}
	gim_info("%s: Successfully added HBM memory region start=0x%llx, size=0x%llx to NUMA node %d\n",
		gim_hbm_drv_mgmt->name, gim_hbm_drv_mgmt->phy_addr, gim_hbm_drv_mgmt->phy_size, gim_hbm_drv_mgmt->numa_id);

	*phy_addr = aligned_addr;
	*phy_size = aligned_size;

	return gim_hbm_drv_mgmt;
}

void gim_hbm_drv_mgmt_fini(void *hbm_drv_mgmt) {
	int ret;
	struct gim_hbm_drv_mgmt_t *gim_hbm_drv_mgmt = (struct gim_hbm_drv_mgmt_t *)hbm_drv_mgmt;
	if (gim_hbm_drv_mgmt != NULL) {
		ret = remove_memory(gim_hbm_drv_mgmt->phy_addr, gim_hbm_drv_mgmt->phy_size);
		if (ret) {
			gim_warn("%s: remove_memory() failed: %d (memory still present. reboot required)\n",
				gim_hbm_drv_mgmt->name, ret);
		} else {
			gim_info("%s: Successfully removed HBM memory region start=0x%llx, size=0x%llx from NUMA node %d\n",
				gim_hbm_drv_mgmt->name, gim_hbm_drv_mgmt->phy_addr, gim_hbm_drv_mgmt->phy_size, gim_hbm_drv_mgmt->numa_id);
		}
		kfree(gim_hbm_drv_mgmt);
	}
}
