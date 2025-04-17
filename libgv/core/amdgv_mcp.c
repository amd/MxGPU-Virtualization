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
#include "amdgv_sched_internal.h"

/* AMDGV Modular Chiplet Platform */
static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

static struct amdgv_id_name spatial_partition_names[] = {
	{ SPATIAL_PARTITION_MODE__SPX, "SPX" }, { SPATIAL_PARTITION_MODE__DPX, "DPX" },
	{ SPATIAL_PARTITION_MODE__TPX, "TPX" }, { SPATIAL_PARTITION_MODE__QPX, "QPX" },
	{ SPATIAL_PARTITION_MODE__CPX, "CPX" },
};

const char *amdgv_mcp_spatial_partition_to_name(enum spatial_partition_mode mode)
{
	int i, count;

	count = ARRAY_SIZE(spatial_partition_names);
	for (i = 0; i < count; ++i) {
		if (spatial_partition_names[i].id == mode)
			return spatial_partition_names[i].name;
	}
	return "UNKNOWN PARTITION MODE";
}

int amdgv_mcp_get_spatial_partition_mode(struct amdgv_adapter *adapt,
					 enum spatial_partition_mode *mode)
{
	if (!adapt->mcp.amdgv_mcp_get_spatial_partition_mode) {
		AMDGV_WARN("Cannot get Spatial Partition mode!\n");
		return AMDGV_FAILURE;
	}

	return adapt->mcp.amdgv_mcp_get_spatial_partition_mode(adapt, mode);
}

int amdgv_mcp_get_num_gfx_spatial_partitions(struct amdgv_adapter *adapt, uint32_t *count)
{
	if (adapt->mcp.amdgv_mcp_get_gfx_num_spatial_partitions) {
		return adapt->mcp.amdgv_mcp_get_gfx_num_spatial_partitions(adapt, count);
	} else {
		*count = 1;
	}

	return 0;
}

int amdgv_mcp_get_num_xcc_per_partition(struct amdgv_adapter *adapt, uint32_t *count)
{
	if (adapt->mcp.gfx.num_xcc_per_partition) {
		*count = adapt->mcp.gfx.num_xcc_per_partition;
	} else {
		AMDGV_WARN("Cannot get number XCC per Partition\n");
		return AMDGV_FAILURE;
	}

	return 0;
}

/* Physical XCC input */
int amdgv_mcp_get_vf_mask_by_xcc(struct amdgv_adapter *adapt, uint32_t idx_xcc)
{
	if (adapt->mcp.get_vf_mask_by_xcc) {
		return adapt->mcp.get_vf_mask_by_xcc(adapt, idx_xcc);
	} else {
		return 0;
	}
}

int amdgv_mcp_get_vf_mask_by_aid(struct amdgv_adapter *adapt, uint32_t idx_aid)
{
	if (adapt->mcp.get_vf_mask_by_aid) {
		return adapt->mcp.get_vf_mask_by_aid(adapt, idx_aid);
	} else {
		return 0;
	}
}

