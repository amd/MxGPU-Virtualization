
/*
 * Copyright (c) 2022 Advanced Micro Devices, Inc. All rights reserved.
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

#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_mcp.h"
#include "mi300/NBIO/nbio_7_9_0_offset.h"
#include "mi300/NBIO/nbio_7_9_0_sh_mask.h"
#include "mi300_gpuiov.h"

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

static int mi308_mcp_set_spatial_partition_mode(struct amdgv_adapter *adapt)
{
	enum spatial_partition_mode mode = SPATIAL_PARTITION_MODE__UNKNOWN;

	switch (adapt->num_vf) {
	case 4:
		mode = SPATIAL_PARTITION_MODE__CPX;
		break;
	case 2:
		mode = SPATIAL_PARTITION_MODE__DPX;
		break;
	case 1:
		mode = SPATIAL_PARTITION_MODE__SPX;
		break;
	default:
		AMDGV_ERROR("Unknown Spatial Partition mode.");
		return AMDGV_FAILURE;
		break;
	}

	adapt->mcp.spatial_partition_mode = mode;
	AMDGV_INFO("Spatial Partition mode: %s\n", amdgv_mcp_spatial_partition_to_name(mode));

	return 0;
}

static int mi308_mcp_set_num_xcc_per_partition(struct amdgv_adapter *adapt)
{
	int num_xcc_per_partition = 0;

	switch (adapt->mcp.spatial_partition_mode) {
	case SPATIAL_PARTITION_MODE__CPX:
		num_xcc_per_partition = 1;
		break;
	case SPATIAL_PARTITION_MODE__DPX:
		num_xcc_per_partition = adapt->mcp.gfx.num_xcc / 2;
		break;
	case SPATIAL_PARTITION_MODE__SPX:
		num_xcc_per_partition = adapt->mcp.gfx.num_xcc;
		break;
	default:
		AMDGV_ERROR("Unknown Spatial Partition mode.");
		return AMDGV_FAILURE;
		break;
	}

	adapt->mcp.gfx.num_xcc_per_partition = num_xcc_per_partition;

	return 0;
}

static int mi308_mcp_get_gfx_num_spatial_partitions(struct amdgv_adapter *adapt,
						    uint32_t *count)
{
	switch (adapt->mcp.spatial_partition_mode) {
	case SPATIAL_PARTITION_MODE__CPX:
		*count = 4;
		break;
	case SPATIAL_PARTITION_MODE__DPX:
		*count = 2;
		break;
	case SPATIAL_PARTITION_MODE__SPX:
		*count = 1;
		break;
	default:
		AMDGV_ERROR("Unknown Spatial Partition mode.");
		return AMDGV_FAILURE;
		break;
	}

	return 0;
}

static int mi308_mcp_get_spatial_partition_mode(struct amdgv_adapter *adapt,
						enum spatial_partition_mode *mode)
{
	switch (adapt->mcp.spatial_partition_mode) {
	case SPATIAL_PARTITION_MODE__CPX:
	case SPATIAL_PARTITION_MODE__DPX:
	case SPATIAL_PARTITION_MODE__SPX:
		*mode = adapt->mcp.spatial_partition_mode;
		break;
	default:
		AMDGV_ERROR("Unknown Spatial Partition mode.");
		return AMDGV_FAILURE;
		break;
	}

	return 0;
}

static int mi308_mcp_get_vf_mask_by_xcc(struct amdgv_adapter *adapt, uint32_t idx_xcc)
{
	uint32_t i = 0;
	uint32_t vf_mask = 0;

	for (i = 0; i < adapt->sched.num_spatial_partitions; i++) {
		if (adapt->sched.spatial_part[i].xcc_mask & (1 << idx_xcc)) {
			vf_mask |= adapt->sched.spatial_part[i].idx_vf_mask;
		}
	}

	return vf_mask;
}

static int mi308_mcp_get_vf_mask_by_aid(struct amdgv_adapter *adapt, uint32_t idx_aid)
{
	uint32_t vf_mask = 0;

	/* on MI308 half XCCs are harvested, so AID id exactly matches XCC id (both logical) */
	vf_mask |= mi308_mcp_get_vf_mask_by_xcc(adapt, idx_aid);

	return vf_mask;
}

static int mi308_mcp_sw_init(struct amdgv_adapter *adapt)
{
	adapt->mcp.amdgv_mcp_get_gfx_num_spatial_partitions =
		mi308_mcp_get_gfx_num_spatial_partitions;
	adapt->mcp.amdgv_mcp_get_spatial_partition_mode = mi308_mcp_get_spatial_partition_mode;
	adapt->mcp.get_vf_mask_by_xcc = mi308_mcp_get_vf_mask_by_xcc;
	adapt->mcp.get_vf_mask_by_aid = mi308_mcp_get_vf_mask_by_aid;

	mi308_mcp_set_spatial_partition_mode(adapt);
	mi308_mcp_set_num_xcc_per_partition(adapt);

	return 0;
}

/*
	Partition (VF num)	Memory NPS	|
						1	2	4	|
	SPX (1)				X		X	|
	DPX (2)				X	 		|
	CPX-4 (4)			X		X	|

	Here the spatial partition mode only refers to vf_num
	and should not be confused with the compute partition mode.
*/
static int mi308_mcp_check_mem_partition_cap(struct amdgv_adapter *adapt)
{
	uint32_t mem_cap = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_PARTITION_MEM_CAP));

	switch (adapt->mcp.spatial_partition_mode) {
	case SPATIAL_PARTITION_MODE__SPX:
	case SPATIAL_PARTITION_MODE__CPX:
		if (!REG_GET_FIELD(mem_cap, BIF_BX_PF0_PARTITION_MEM_CAP, NPS1_SUPPORT) &&
		    !REG_GET_FIELD(mem_cap, BIF_BX_PF0_PARTITION_MEM_CAP, NPS4_SUPPORT)) {
			goto failure;
		}
		break;
	case SPATIAL_PARTITION_MODE__DPX:
		if (!REG_GET_FIELD(mem_cap, BIF_BX_PF0_PARTITION_MEM_CAP, NPS1_SUPPORT)) {
			goto failure;
		}
		break;
	default:
		goto failure;
		break;
	}

	return 0;

failure:
	AMDGV_ERROR("Selected Partition Mode is not supported!\n");
	return AMDGV_FAILURE;
}

/*
	Partition (VF num)	Compute		|
						SPX	DPX	CPX	|
	SPX (1)				X		X	|
	DPX (2)					X	X	|
	CPX-4 (4)					X	|

	Here the spatial partition mode only refers to vf_num
	and should not be confused with the compute partition mode.
*/
static int mi308_mcp_check_compute_partition_cap(struct amdgv_adapter *adapt)
{
	uint32_t compute_cap =
		RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_PARTITION_COMPUTE_CAP));

	switch (adapt->mcp.spatial_partition_mode) {
	case SPATIAL_PARTITION_MODE__SPX:
		if (!REG_GET_FIELD(compute_cap, BIF_BX_PF0_PARTITION_COMPUTE_CAP,
				   SPX_SUPPORT) &&
			!REG_GET_FIELD(compute_cap, BIF_BX_PF0_PARTITION_COMPUTE_CAP,
				   CPX_SUPPORT)) {
			goto failure;
		}
		break;
	case SPATIAL_PARTITION_MODE__DPX:
		if (!REG_GET_FIELD(compute_cap, BIF_BX_PF0_PARTITION_COMPUTE_CAP,
				   QPX_SUPPORT) &&
			!REG_GET_FIELD(compute_cap, BIF_BX_PF0_PARTITION_COMPUTE_CAP,
				   CPX_SUPPORT)) {
			goto failure;
		}
		break;
	case SPATIAL_PARTITION_MODE__CPX:
		if (!REG_GET_FIELD(compute_cap, BIF_BX_PF0_PARTITION_COMPUTE_CAP,
				   CPX_SUPPORT)) {
			goto failure;
		}
		break;
	default:
		goto failure;
		break;
	}

	return 0;

failure:
	AMDGV_ERROR("Selected Partition Mode is not supported!\n");
	return AMDGV_FAILURE;
}

static int mi308_mcp_hw_init(struct amdgv_adapter *adapt)
{
	int r;

	r = mi308_mcp_check_compute_partition_cap(adapt);
	if (r)
		return r;

	r = mi308_mcp_check_mem_partition_cap(adapt);
	if (r)
		return r;

	return 0;
}

static int mi308_mcp_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->mcp.amdgv_mcp_get_gfx_num_spatial_partitions = NULL;
	adapt->mcp.amdgv_mcp_get_spatial_partition_mode = NULL;
	adapt->mcp.get_vf_mask_by_xcc = NULL;
	adapt->mcp.get_vf_mask_by_aid = NULL;

	return 0;
}

static int mi308_mcp_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func mi308_mcp_func = {
	.name = "mi308_mcp_func",
	.sw_init = mi308_mcp_sw_init,
	.sw_fini = mi308_mcp_sw_fini,
	.hw_init = mi308_mcp_hw_init,
	.hw_fini = mi308_mcp_hw_fini,
};
