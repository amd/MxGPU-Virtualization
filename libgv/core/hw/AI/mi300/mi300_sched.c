/*
 * Copyright (c) 2021 Advanced Micro Devices, Inc. All rights reserved.
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

#include <amdgv_device.h>
#include <amdgv_mcp.h>
#include <amdgv_sched.h>
#include <amdgv_sched_internal.h>

#include "mi300.h"
#include "mi300/GC/gc_9_4_3_offset.h"
#include "mi300/GC/gc_9_4_3_sh_mask.h"
#include "mi300_gpuiov.h"
#include "mi300_psp.h"

static const uint32_t this_block = AMDGV_SCHEDULER_BLOCK;

static struct amdgv_sched_spatial_part mi300_block_configs_8x8
	[SPATIAL_PARTITION_MODE__NUM_MODES][AMDGV_SCHED_SPATIAL_PARTITION_MAX] = {
		[SPATIAL_PARTITION_MODE__SPX] = {			// SPX
			[AMDGV_SCHED_SPATIAL_PARTITION_0] = {
				.idx_vf_mask = BIT(0) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH0_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH1_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH2_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH3_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH4_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH5_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH6_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH7_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH0_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH0_MMSCH)	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH1_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH1_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH1_MMSCH)	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH2_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH2_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH2_MMSCH)	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH3_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH3_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH3_MMSCH)),
			},
		},
		[SPATIAL_PARTITION_MODE__DPX] = {			// DPX
			[AMDGV_SCHED_SPATIAL_PARTITION_0] = {
				.idx_vf_mask = BIT(0) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH0_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH1_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH2_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH3_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH0_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH0_MMSCH)	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH1_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH1_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH1_MMSCH)),
			},
			[AMDGV_SCHED_SPATIAL_PARTITION_1] = {
				.idx_vf_mask = BIT(1) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH4_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH5_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH6_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH7_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH2_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH2_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH2_MMSCH)	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH3_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH3_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH3_MMSCH)),
			},
		},
		[SPATIAL_PARTITION_MODE__TPX] = {			// TPX - Not supported
			[AMDGV_SCHED_SPATIAL_PARTITION_0] = {0}
		},
		[SPATIAL_PARTITION_MODE__QPX] = {			// QPX
			[AMDGV_SCHED_SPATIAL_PARTITION_0] = {
				.idx_vf_mask = BIT(0) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH0_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH1_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH0_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH0_MMSCH)),
			},
			[AMDGV_SCHED_SPATIAL_PARTITION_1] = {
				.idx_vf_mask = BIT(1) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH2_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH3_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH1_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH1_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH1_MMSCH)),
			},
			[AMDGV_SCHED_SPATIAL_PARTITION_2] = {
				.idx_vf_mask = BIT(2) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH4_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH5_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH2_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH2_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH2_MMSCH)),
			},
			[AMDGV_SCHED_SPATIAL_PARTITION_3] = {
				.idx_vf_mask = BIT(3) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH6_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH7_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH3_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH3_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH3_MMSCH)),
			},
		},
		[SPATIAL_PARTITION_MODE__CPX] = {			// CPX-8
			[AMDGV_SCHED_SPATIAL_PARTITION_0] = {
				.idx_vf_mask = BIT(0) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH0_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH0_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH0_MMSCH)),
			},
			[AMDGV_SCHED_SPATIAL_PARTITION_1] = {
				.idx_vf_mask = BIT(1) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH1_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH0_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH0_MMSCH)),
			},
			[AMDGV_SCHED_SPATIAL_PARTITION_2] = {
				.idx_vf_mask = BIT(2) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH2_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH1_MMSCH)	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH1_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH1_MMSCH)),
			},
			[AMDGV_SCHED_SPATIAL_PARTITION_3] = {
				.idx_vf_mask = BIT(3) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH3_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH1_MMSCH)	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH1_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH1_MMSCH)),
			},
			[AMDGV_SCHED_SPATIAL_PARTITION_4] = {
				.idx_vf_mask = BIT(4) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH4_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH2_MMSCH)	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH2_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH2_MMSCH)),
			},
			[AMDGV_SCHED_SPATIAL_PARTITION_5] = {
				.idx_vf_mask = BIT(5) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH5_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH2_MMSCH)	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH2_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH2_MMSCH)),
			},
			[AMDGV_SCHED_SPATIAL_PARTITION_6] = {
				.idx_vf_mask = BIT(6) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH6_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH3_MMSCH)	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH3_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH3_MMSCH)),
			},
			[AMDGV_SCHED_SPATIAL_PARTITION_7] = {
				.idx_vf_mask = BIT(7) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH7_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH3_MMSCH)	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH3_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH3_MMSCH)),
			},
		},
	};

static struct amdgv_sched_spatial_part mi308_block_configs
	[SPATIAL_PARTITION_MODE__NUM_MODES][AMDGV_SCHED_SPATIAL_PARTITION_MAX] = {
		[SPATIAL_PARTITION_MODE__SPX] = {			// SPX
			[AMDGV_SCHED_SPATIAL_PARTITION_0] = {
				.idx_vf_mask = BIT(0) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH0_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH1_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH2_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH3_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH0_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH0_MMSCH)	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH1_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH1_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH1_MMSCH)	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH2_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH2_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH2_MMSCH)	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH3_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH3_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH3_MMSCH)),
			},
		},
		[SPATIAL_PARTITION_MODE__DPX] = {			// DPX
			[AMDGV_SCHED_SPATIAL_PARTITION_0] = {
				.idx_vf_mask = BIT(0) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH0_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH1_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH0_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH0_MMSCH)	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH1_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH1_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH1_MMSCH)),
			},
			[AMDGV_SCHED_SPATIAL_PARTITION_1] = {
				.idx_vf_mask = BIT(1) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH2_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_GFX_SCH3_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH2_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH2_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH2_MMSCH)	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH3_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH3_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH3_MMSCH)),
			},
		},
		[SPATIAL_PARTITION_MODE__TPX] = {			// TPX - Not supported
			[AMDGV_SCHED_SPATIAL_PARTITION_0] = {0}
		},
		[SPATIAL_PARTITION_MODE__QPX] = {			// QPX - Not supported
			[AMDGV_SCHED_SPATIAL_PARTITION_0] = {0}
		},
		[SPATIAL_PARTITION_MODE__CPX] = {			// CPX-4
			[AMDGV_SCHED_SPATIAL_PARTITION_0] = {
				.idx_vf_mask = BIT(0) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH0_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH0_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH0_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH0_MMSCH)),
			},
			[AMDGV_SCHED_SPATIAL_PARTITION_1] = {
				.idx_vf_mask = BIT(1) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH1_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH1_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH1_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH1_MMSCH)),
			},
			[AMDGV_SCHED_SPATIAL_PARTITION_2] = {
				.idx_vf_mask = BIT(2) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH2_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH2_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH2_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH2_MMSCH)),
			},
			[AMDGV_SCHED_SPATIAL_PARTITION_3] = {
				.idx_vf_mask = BIT(3) | BIT(AMDGV_PF_IDX),
				.hw_sched_mask =
					(BIT(MI300_HW_SCHED_BLOCK_GFX_SCH3_RLCV) 	|
					 BIT(MI300_HW_SCHED_BLOCK_VCN_SCH3_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG_SCH3_MMSCH) 	|
					 BIT(MI300_HW_SCHED_BLOCK_JPEG1_SCH3_MMSCH)),
			},
		},
	};


static void mi300_sched_dump_gpu_state(struct amdgv_adapter *adapt)
{
}

static uint32_t mi300_cp_sched_state(struct amdgv_adapter *adapt, uint32_t idx_vf)
{
	uint32_t xcc_id;
	uint32_t ret = 0;

	// W/A until bringup PF programming is complete
	if (idx_vf == AMDGV_PF_IDX)
		return 1;

	for_each_id (xcc_id, amdgv_sched_get_xcc_mask_by_vf(adapt, idx_vf)) {
		ret |= RREG32(SOC15_REG_OFFSET(GC, GET_INST(GC, xcc_id), regRLC_CP_SCHEDULERS));
	}

	return ret;
}

static int mi300_sched_copy_static_spatial_part_table(struct amdgv_adapter *adapt,
						      uint32_t num_vf)
{
	int r, i;
	enum spatial_partition_mode mode;
	uint32_t gfx_partition_count;
	static struct amdgv_sched_spatial_part *table;

	oss_memset(adapt->sched.spatial_part, 0, sizeof(struct amdgv_sched_spatial_part));

	adapt->sched.num_spatial_partitions = num_vf;

	if (amdgv_mcp_get_num_gfx_spatial_partitions(adapt, &gfx_partition_count))
		return AMDGV_FAILURE;

	adapt->sched.num_vf_per_gfx_sched = num_vf / gfx_partition_count;

	r = amdgv_mcp_get_spatial_partition_mode(adapt, &mode);
	if (r)
		return r;

	switch (adapt->dev_id) {
	case (0x74A1):
	case (0x74A9):
	case (0x74A5):
	case (0x0070):
	case (0x0071):
	case (0x75A0):
	case (0x75A1):
	case (0x75A3):
		table = mi300_block_configs_8x8[mode];
		break;
	case (0x74a2):
	case (0x74a8):
		table = mi308_block_configs[mode];
		break;
	default:
		AMDGV_ERROR("Invalid or unsupported device ID!\n");
		return AMDGV_FAILURE;
	}

	for (i = 0; i < adapt->sched.num_spatial_partitions; i++) {
		adapt->sched.spatial_part[i].hw_sched_mask = table[i].hw_sched_mask;
		/* Remove all invalid VF bits */
		adapt->sched.spatial_part[i].idx_vf_mask =
			((table[i].idx_vf_mask & ((1 << num_vf) - 1)) | (1 << AMDGV_PF_IDX));

		adapt->sched.spatial_part[i].xcc_mask =
			(adapt->sched.spatial_part[i].hw_sched_mask >>
			 MI300_HW_SCHED_BLOCK_GFX_SCH0_RLCV);
	}

	return 0;
}

static int mi300_sched_reconfig_mapping_tables(struct amdgv_adapter *adapt, uint32_t num_vf)
{
	AMDGV_ERROR("MI300 CANNOT DYNAMICALLY CHANGE VF NUMBER YET!\n");

	return AMDGV_FAILURE;

	// if (mi300_sched_copy_static_spatial_part_table(adapt, num_vf))
	// 	return AMDGV_FAILURE;

	// if (amdgv_sched_world_switch_remap_vf_assignment(adapt))
	// 	return AMDGV_FAILURE;

	// if (amdgv_sched_part_mapping_init(adapt))
	// 	return AMDGV_FAILURE;
}

static int mi300_sched_sw_init(struct amdgv_adapter *adapt)
{
	adapt->sched.dump_gpu_state = mi300_sched_dump_gpu_state;
	adapt->sched.cp_sched_state = mi300_cp_sched_state;
	adapt->sched.get_asic_time_slice = amdgv_sched_get_asic_time_slice;
	adapt->sched.reconfig_mapping_tables = mi300_sched_reconfig_mapping_tables;

	adapt->sched.enable_bulk_goto_state = true;

	/* enable bar protection scheme */
	adapt->flags |= AMDGV_FLAG_VF_FB_PROTECTION;

	/* enable (by default) clearing vf fb region for Mi300 */
	if (!(adapt->flags & AMDGV_FLAG_USE_PF))
		adapt->flags |= AMDGV_FLAG_ENABLE_CLEAR_VF_FB;

	/* check partition full access enable for MI300X/MI308X*/
	if (adapt->flags & AMDGV_FLAG_ENABLE_PARTITION_FULL_ACCESS)
		adapt->sched.enable_per_partition_full_access = true;
	else
		AMDGV_INFO("partition full access is disabled\n");

	if (mi300_sched_copy_static_spatial_part_table(adapt, adapt->num_vf))
		return AMDGV_FAILURE;

	if (amdgv_sched_init(adapt))
		return AMDGV_FAILURE;

	if (adapt->opt.allow_time_full_access == 0) {
		adapt->sched.allow_time_full_access = 20000 * 1000;
		AMDGV_DEBUG("allowed time for full access has been changed to %dms for MI300 series\n",
					adapt->sched.allow_time_full_access / 1000);
	}

	return 0;
}

static int mi300_sched_sw_fini(struct amdgv_adapter *adapt)
{
	amdgv_sched_fini(adapt);

	return 0;
}

static int mi300_sched_hw_init_early(struct amdgv_adapter *adapt)
{
	return amdgv_sched_init_pf_state_early(adapt);
}

static int mi300_sched_hw_init_late(struct amdgv_adapter *adapt)
{
	return amdgv_sched_init_pf_state_late(adapt);
}


static int mi300_sched_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

static int mi300_sched_sw_init_early(struct amdgv_adapter *adapt) { return mi300_sched_sw_init(adapt); }
static int mi300_sched_sw_fini_early(struct amdgv_adapter *adapt) { return mi300_sched_sw_fini(adapt); }
static int mi300_sched_hw_fini_early(struct amdgv_adapter *adapt) { return mi300_sched_hw_fini(adapt); }

static int mi300_sched_sw_init_late(struct amdgv_adapter *adapt) { return 0; }
static int mi300_sched_sw_fini_late(struct amdgv_adapter *adapt) { return 0; }
static int mi300_sched_hw_fini_late(struct amdgv_adapter *adapt) { return 0; }

struct amdgv_init_func mi300_sched_early_func = {
    .name = "mi300_sched_func_early",
    .sw_init = mi300_sched_sw_init_early,
    .sw_fini = mi300_sched_sw_fini_early,
    .hw_init = mi300_sched_hw_init_early,
    .hw_fini = mi300_sched_hw_fini_early,
};

struct amdgv_init_func mi300_sched_late_func = {
    .name = "mi300_sched_func_late",
    .sw_init = mi300_sched_sw_init_late,
    .sw_fini = mi300_sched_sw_fini_late,
    .hw_init = mi300_sched_hw_init_late,
    .hw_fini = mi300_sched_hw_fini_late,
};
