/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv_sched.h>
#include <amdgv_sched_internal.h>

#include "mi200.h"
#include "psp_v13_0.h"
#include "mi200_gpuiov.h"


static const uint32_t this_block = AMDGV_SCHEDULER_BLOCK;

static struct amdgv_sched_spatial_part mi200_sched_config_tbl = {
	.idx_vf_mask = AMDGV_SCHED_ALLOWED_VF_ASSIGNMENT_ALL,
	.hw_sched_mask =  ((1 << MI200_HW_SCHED_BLOCK_UVD_SCH0_MMSCH) |
						(1 << MI200_HW_SCHED_BLOCK_GFX_SCH0_RLCV) |
						(1 << MI200_HW_SCHED_BLOCK_UVD_SCH1_MMSCH))
};

static void mi200_sched_dump_gpu_state(struct amdgv_adapter *adapt)
{

}

static uint32_t mi200_cp_sched_state(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t ret = RREG32(SOC15_REG_OFFSET(GC, 0, mmRLC_CP_SCHEDULERS));
	return ret;
}

static int mi200_sched_copy_static_spatial_part_table(struct amdgv_adapter *adapt, uint32_t num_vf)
{
	oss_memset(adapt->sched.spatial_part, 0, sizeof(struct amdgv_sched_spatial_part));

	//MI200 only supports a single spatial partition
	adapt->sched.num_vf_per_gfx_sched = num_vf;
	adapt->sched.num_spatial_partitions = 1;
	adapt->sched.spatial_part[0].hw_sched_mask = mi200_sched_config_tbl.hw_sched_mask;
	/* Remove all invalid VF bits */
	adapt->sched.spatial_part[0].idx_vf_mask =
		((mi200_sched_config_tbl.idx_vf_mask & ((1 << num_vf) - 1)) | (1 << AMDGV_PF_IDX));

	//Only single GC block instance
	adapt->sched.spatial_part[0].xcc_mask = (1 << 0);

	return 0;
}

static int mi200_sched_reconfig_mapping_tables(struct amdgv_adapter *adapt, uint32_t num_vf)
{
	if (mi200_sched_copy_static_spatial_part_table(adapt, num_vf))
		return AMDGV_FAILURE;

	if (amdgv_sched_world_switch_remap_vf_assignment(adapt))
		return AMDGV_FAILURE;

	if (amdgv_sched_part_mapping_init(adapt))
		return AMDGV_FAILURE;

	return 0;
}

static int mi200_sched_sw_init(struct amdgv_adapter *adapt)
{
	adapt->sched.dump_gpu_state = mi200_sched_dump_gpu_state;
	adapt->sched.cp_sched_state = mi200_cp_sched_state;
	adapt->sched.get_asic_time_slice = amdgv_sched_get_asic_time_slice;
	adapt->sched.reconfig_mapping_tables = mi200_sched_reconfig_mapping_tables;
	/* enable (by default) clearing vf fb region for mi200 */
	if (!(adapt->flags & AMDGV_FLAG_USE_PF))
		adapt->flags |= AMDGV_FLAG_ENABLE_CLEAR_VF_FB;

	if (mi200_sched_copy_static_spatial_part_table(adapt, adapt->num_vf))
		return AMDGV_FAILURE;

	return amdgv_sched_init(adapt);
}

static int mi200_sched_sw_fini(struct amdgv_adapter *adapt)
{
	amdgv_sched_fini(adapt);

	return 0;
}

static int mi200_sched_hw_init(struct amdgv_adapter *adapt)
{
	int r;

	r = amdgv_sched_init_pf_state(adapt);
	if (r)
		return r;

	return r;
}

static int mi200_sched_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

struct amdgv_init_func mi200_sched_func = {
	.name = "mi200_sched_func",
	.sw_init = mi200_sched_sw_init,
	.sw_fini = mi200_sched_sw_fini,
	.hw_init = mi200_sched_hw_init,
	.hw_fini = mi200_sched_hw_fini,
};
