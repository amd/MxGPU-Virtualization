/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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
#include <linux/workqueue.h>
#include "gim_debug.h"
#include "gim_hbm_drv_mgmt.h"

#define GIM_HBM_DRV_MGMT_NAME_LEN 32

/*
 * Defer HBM NUMA registration so add_memory_driver_managed() does not run
 * during VF enable while cgroup users may be iterating tasks.
 * Some platform and kernel 6.14 combinations can WARN/hang when zonelists
 * rebuild races cgroup scan.
 */
static unsigned int hbm_reg_delay_ms = 5000;
module_param(hbm_reg_delay_ms, uint, 0644);
MODULE_PARM_DESC(hbm_reg_delay_ms,
	"Delay before registering VF HBM with NUMA via add_memory_driver_managed (ms). "
	"0 schedules registration immediately on a dedicated workqueue.");

static struct workqueue_struct *gim_hbm_wq;

struct gim_hbm_drv_mgmt_t {
	char name[GIM_HBM_DRV_MGMT_NAME_LEN];
	char vf_name[GIM_HBM_DRV_MGMT_NAME_LEN];
	int numa_id;
	uint64_t phy_addr;
	uint64_t phy_size;
	struct delayed_work reg_work;
	bool registered;
};

static int gim_hbm_drv_mgmt_add_memory(struct gim_hbm_drv_mgmt_t *ctx)
{
	int ret;

	ret = add_memory_driver_managed(ctx->numa_id, ctx->phy_addr, ctx->phy_size,
					ctx->name, MHP_MEMMAP_ON_MEMORY);
	if (ret == -EINVAL) {
		gim_info("%s: MHP_MEMMAP_ON_MEMORY unsupported, retry with MHP_NONE\n",
			 ctx->vf_name);
		ret = add_memory_driver_managed(ctx->numa_id, ctx->phy_addr, ctx->phy_size,
						ctx->name, MHP_NONE);
	}

	return ret;
}

static void gim_hbm_drv_mgmt_reg_work(struct work_struct *work)
{
	struct gim_hbm_drv_mgmt_t *ctx =
		container_of(to_delayed_work(work), struct gim_hbm_drv_mgmt_t, reg_work);
	int ret;

	ret = gim_hbm_drv_mgmt_add_memory(ctx);
	if (ret) {
		gim_warn("%s: Failed to add HBM memory to kernel memory management (error: %d)\n",
			 ctx->vf_name, ret);
		if (ret == -EEXIST)
			gim_warn("%s: Memory region already exists or overlaps with existing memory\n",
				 ctx->vf_name);
		else if (ret == -EINVAL)
			gim_warn("%s: Invalid memory region params (check alignment and address range)\n",
				 ctx->vf_name);
		else if (ret == -ENOMEM)
			gim_warn("%s: Insufficient memory for memory management structures\n",
				 ctx->vf_name);
		return;
	}

	ctx->registered = true;
	gim_info("%s: Successfully added HBM memory region start=0x%llx, size=0x%llx to NUMA node %d\n",
		 ctx->name, ctx->phy_addr, ctx->phy_size, ctx->numa_id);
}

int gim_hbm_drv_mgmt_module_init(void)
{
	gim_hbm_wq = alloc_workqueue("gim_hbm", WQ_UNBOUND | WQ_MEM_RECLAIM, 1);
	if (!gim_hbm_wq) {
		gim_warn("failed to create HBM registration workqueue\n");
		return -ENOMEM;
	}

	return 0;
}

void gim_hbm_drv_mgmt_module_fini(void)
{
	if (gim_hbm_wq) {
		flush_workqueue(gim_hbm_wq);
		destroy_workqueue(gim_hbm_wq);
		gim_hbm_wq = NULL;
	}
}

void *gim_hbm_drv_mgmt_init(const char *name, int numa_id,
			    uint64_t *phy_addr, uint64_t *phy_size)
{
	struct gim_hbm_drv_mgmt_t *ctx;
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

	if (!gim_hbm_wq) {
		gim_warn("%s: HBM registration workqueue is not initialized\n", name);
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

	ctx = kzalloc(sizeof(*ctx), GFP_KERNEL);
	if (ctx == NULL) {
		gim_warn("failed to allocate memory for gim hbm mgmt\n");
		return NULL;
	}

	ctx->numa_id = numa_id;
	ctx->phy_addr = aligned_addr;
	ctx->phy_size = aligned_size;
	snprintf(ctx->name, sizeof(ctx->name), "System RAM (%s)", name);
	snprintf(ctx->vf_name, sizeof(ctx->vf_name), "%s", name);

	INIT_DELAYED_WORK(&ctx->reg_work, gim_hbm_drv_mgmt_reg_work);
	queue_delayed_work(gim_hbm_wq, &ctx->reg_work,
			     msecs_to_jiffies(hbm_reg_delay_ms));

	gim_info("%s: scheduled HBM NUMA registration on node %d in %u ms\n",
		 name, numa_id, hbm_reg_delay_ms);

	*phy_addr = aligned_addr;
	*phy_size = aligned_size;

	return ctx;
}

void gim_hbm_drv_mgmt_fini(void *hbm_drv_mgmt)
{
	struct gim_hbm_drv_mgmt_t *ctx = hbm_drv_mgmt;

	if (ctx != NULL) {
		cancel_delayed_work_sync(&ctx->reg_work);
		if (ctx->registered) {
			int ret;

			ret = remove_memory(ctx->phy_addr, ctx->phy_size);
			if (ret) {
				gim_warn("%s: remove_memory() failed: %d (memory still present. reboot required)\n",
					 ctx->name, ret);
			} else {
				gim_info("%s: Successfully removed HBM memory region start=0x%llx, size=0x%llx from NUMA node %d\n",
					 ctx->name, ctx->phy_addr, ctx->phy_size, ctx->numa_id);
			}
		}
		kfree(ctx);
	}
}
