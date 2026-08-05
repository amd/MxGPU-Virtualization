/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include <amdgv_mcp.h>
#include <amdgv_sched.h>
#include <amdgv_sched_internal.h>

#include "asic_reg/GC/gc_12_1_0_offset.h"
#include "asic_reg/GC/gc_12_1_0_sh_mask.h"
#include "gpuiov_v9_0.h"

static const uint32_t this_block = AMDGV_SCHEDULER_BLOCK;

/* GFX RLCV scheduler blocks, indexed by physical XCC instance. */
static const uint32_t gpuiov_v9_0_gfx_sched_bits[GPUIOV_V9_0_MAX_XCD_NUM] = {
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_GFX_SCH0_RLCV),
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_GFX_SCH1_RLCV),
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_GFX_SCH2_RLCV),
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_GFX_SCH3_RLCV),
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_GFX_SCH4_RLCV),
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_GFX_SCH5_RLCV),
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_GFX_SCH6_RLCV),
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_GFX_SCH7_RLCV),
};

/* Each VCN MMSCH block drives its VCN plus two JPEG engines, and is indexed by
 * VCN instance. A fixed group of consecutive physical XCCs aligns to each VCN.
 */
static const uint32_t gpuiov_v9_0_vcn_sched_bits[GPUIOV_V9_0_MAX_VCN_NUM] = {
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_VCN_SCH0_MMSCH) |
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) |
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG1_SCH0_MMSCH),
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_VCN_SCH1_MMSCH) |
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG_SCH1_MMSCH) |
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG1_SCH1_MMSCH),
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_VCN_SCH2_MMSCH) |
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG_SCH2_MMSCH) |
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG1_SCH2_MMSCH),
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_VCN_SCH3_MMSCH) |
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG_SCH3_MMSCH) |
	BIT(GPUIOV_V9_0_HW_SCHED_BLOCK_JPEG1_SCH3_MMSCH),
};

/* Encode the shared per-partition physical XCC/VCN layout into this ASIC's
 * hw_sched blocks: GFX RLCV per XCC and the VCN/JPEG MMSCH group per VCN.
 */
int gpuiov_v9_0_sched_compute_spatial_part_table(struct amdgv_sched_spatial_part *table,
						 uint32_t xcc_mask, uint32_t num_vf)
{
	struct amdgv_spatial_partition_layout layout;
	uint32_t i, id, hw_sched_mask;

	if (amdgv_mcp_build_partition_layout(xcc_mask, GPUIOV_V9_0_MAX_XCD_NUM,
					     GPUIOV_V9_0_MAX_VCN_NUM, num_vf, &layout))
		return AMDGV_FAILURE;

	for (i = 0; i < layout.num_partitions; i++) {
		hw_sched_mask = 0;
		for_each_id (id, layout.part[i].xcc_mask)
			hw_sched_mask |= gpuiov_v9_0_gfx_sched_bits[id];
		for_each_id (id, layout.part[i].vcn_mask)
			hw_sched_mask |= gpuiov_v9_0_vcn_sched_bits[id];

		table[i].hw_sched_mask = hw_sched_mask;
		table[i].idx_vf_mask = BIT(i) | BIT(AMDGV_PF_IDX);
		table[i].xcc_mask = layout.part[i].xcc_mask;
	}

	return 0;
}

static void gpuiov_v9_0_sched_dump_gpu_state(struct amdgv_adapter *adapt)
{
}

static uint32_t gpuiov_v9_0_cp_sched_state(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t xcc_id;
	uint32_t ret = 0;

	for_each_id (xcc_id, amdgv_sched_get_xcc_mask_by_vf(adapt, idx_vf)) {
		ret |= RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regRLC_CP_SCHEDULERS));
	}

	return ret;
}

static int gpuiov_v9_0_sched_setup_spatial_part_table(struct amdgv_adapter *adapt,
						      uint32_t num_vf)
{
	uint32_t gfx_partition_count;

	oss_memset(adapt->sched.spatial_part, 0, sizeof(struct amdgv_sched_spatial_part));

	adapt->sched.num_spatial_partitions = num_vf;

	if (amdgv_mcp_get_num_gfx_spatial_partitions(adapt, &gfx_partition_count))
		return AMDGV_FAILURE;

	adapt->sched.num_vf_per_gfx_sched = num_vf / gfx_partition_count;

	if (gpuiov_v9_0_sched_compute_spatial_part_table(adapt->sched.spatial_part,
							 adapt->mcp.gfx.xcc_mask, num_vf)) {
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_SCHED_INVALID_XCC_NUM,
			      adapt->mcp.gfx.num_xcc);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int gpuiov_v9_0_sched_reconfig_mapping_tables(struct amdgv_adapter *adapt, uint32_t num_vf)
{
	AMDGV_ERROR("CANNOT DYNAMICALLY CHANGE VF NUMBER YET!\n");

	return AMDGV_FAILURE;

	// if (gpuiov_v9_0_sched_setup_spatial_part_table(adapt, num_vf))
	// 	return AMDGV_FAILURE;

	// if (amdgv_sched_world_switch_remap_vf_assignment(adapt))
	// 	return AMDGV_FAILURE;

	// if (amdgv_sched_part_mapping_init(adapt))
	// 	return AMDGV_FAILURE;
}

static int gpuiov_v9_0_sched_sw_init(struct amdgv_adapter *adapt)
{
	adapt->sched.dump_gpu_state = gpuiov_v9_0_sched_dump_gpu_state;
	adapt->sched.cp_sched_state = gpuiov_v9_0_cp_sched_state;
	adapt->sched.get_asic_time_slice = amdgv_sched_get_asic_time_slice;
	adapt->sched.reconfig_mapping_tables = gpuiov_v9_0_sched_reconfig_mapping_tables;

	adapt->sched.enable_bulk_goto_state = true;

	/* enable bar protection scheme */
	adapt->flags |= AMDGV_FLAG_VF_FB_PROTECTION;
	adapt->flags |= AMDGV_FLAG_NO_DYNAMIC_VF_NUM;


	/* enable (by default) clearing vf fb region for GC v12.1 */
	if (!(adapt->flags & AMDGV_FLAG_USE_PF))
		adapt->flags |= AMDGV_FLAG_ENABLE_CLEAR_VF_FB;

	/* check partition full access enable for GC v12.1 */
	if (adapt->flags & AMDGV_FLAG_ENABLE_PARTITION_FULL_ACCESS)
		adapt->sched.enable_per_partition_full_access = true;
	else
		amdgv_put_log(AMDGV_PF_IDX,
			      AMDGV_LOG_SCHED_PARTITION_FULL_ACCESS_DISABLED, 0);

	if (gpuiov_v9_0_sched_setup_spatial_part_table(adapt, adapt->num_vf))
		return AMDGV_FAILURE;

	if (amdgv_sched_init(adapt))
		return AMDGV_FAILURE;

	if (adapt->opt.allow_time_full_access == 0) {
		adapt->sched.allow_time_full_access = 20000 * 1000;
		amdgv_put_log(AMDGV_PF_IDX, AMDGV_LOG_SCHED_FULL_ACCESS_TIME_CHANGED,
			      adapt->sched.allow_time_full_access / 1000);
	}

	return 0;
}

static int gpuiov_v9_0_sched_sw_fini(struct amdgv_adapter *adapt)
{
	amdgv_sched_fini(adapt);

	return 0;
}

static int gpuiov_v9_0_sched_hw_init_early(struct amdgv_adapter *adapt)
{
	return amdgv_sched_init_pf_state_early(adapt);
}

static int gpuiov_v9_0_sched_hw_init_late(struct amdgv_adapter *adapt)
{
	return amdgv_sched_init_pf_state_late(adapt);
}

static int gpuiov_v9_0_sched_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int gpuiov_v9_0_sched_sw_init_early(struct amdgv_adapter *adapt) { return gpuiov_v9_0_sched_sw_init(adapt); }
static int gpuiov_v9_0_sched_sw_fini_early(struct amdgv_adapter *adapt) { return gpuiov_v9_0_sched_sw_fini(adapt); }
static int gpuiov_v9_0_sched_hw_fini_early(struct amdgv_adapter *adapt) { return gpuiov_v9_0_sched_hw_fini(adapt); }

static int gpuiov_v9_0_sched_sw_init_late(struct amdgv_adapter *adapt) { return 0; }
static int gpuiov_v9_0_sched_sw_fini_late(struct amdgv_adapter *adapt) { return 0; }
static int gpuiov_v9_0_sched_hw_fini_late(struct amdgv_adapter *adapt) { return 0; }

struct amdgv_init_func gpuiov_v9_0_sched_early_func = {
    .name = "gpuiov_v9_0_sched_func_early",
    .sw_init = gpuiov_v9_0_sched_sw_init_early,
    .sw_fini = gpuiov_v9_0_sched_sw_fini_early,
    .hw_init = gpuiov_v9_0_sched_hw_init_early,
    .hw_fini = gpuiov_v9_0_sched_hw_fini_early,
};

struct amdgv_init_func gpuiov_v9_0_sched_late_func = {
    .name = "gpuiov_v9_0_sched_func_late",
    .sw_init = gpuiov_v9_0_sched_sw_init_late,
    .sw_fini = gpuiov_v9_0_sched_sw_fini_late,
    .hw_init = gpuiov_v9_0_sched_hw_init_late,
    .hw_fini = gpuiov_v9_0_sched_hw_fini_late,
};
