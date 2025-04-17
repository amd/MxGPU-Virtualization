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

#ifndef __AMDGV_MCP_H__
#define __AMDGV_MCP_H__


enum spatial_partition_mode {
	SPATIAL_PARTITION_MODE__SPX		= 0,
	SPATIAL_PARTITION_MODE__DPX		= 1,
	SPATIAL_PARTITION_MODE__TPX		= 2,
	SPATIAL_PARTITION_MODE__QPX		= 3,
	SPATIAL_PARTITION_MODE__CPX		= 4,
	SPATIAL_PARTITION_MODE__NUM_MODES	= 5,
	SPATIAL_PARTITION_MODE__UNKNOWN
};

struct spatial_partition_gfx {
	uint16_t	xcc_mask;
	uint16_t	num_xcc;
	uint32_t	num_xcc_per_partition;
	uint32_t	sdma_mask;
};

struct amdgv_mcp {
	uint32_t num_aid;
	enum spatial_partition_mode spatial_partition_mode;
	uint32_t accelerator_partition_mode;
	enum amdgv_memory_partition_mode memory_partition_mode;
	bool mem_mode_switch_requested;
	uint32_t numa_count;
	struct {
		uint64_t start;
		uint64_t end;
	} numa_range[AMDGV_MAX_NUMA_NODES];
	struct spatial_partition_gfx gfx;

	int (*amdgv_mcp_get_gfx_num_spatial_partitions)(
		struct amdgv_adapter *adapt, uint32_t *count);
	int (*amdgv_mcp_get_spatial_partition_mode)(struct amdgv_adapter *adapt, enum spatial_partition_mode *mode);
	int (*get_vf_mask_by_xcc)(struct amdgv_adapter *adapt,  uint32_t idx_xcc);
	int (*get_vf_mask_by_aid)(struct amdgv_adapter *adapt,  uint32_t idx_aid);
};
const char *amdgv_mcp_spatial_partition_to_name(enum spatial_partition_mode mode);
int amdgv_mcp_get_num_gfx_spatial_partitions(struct amdgv_adapter *adapt, uint32_t *count);
int amdgv_mcp_get_spatial_partition_mode(struct amdgv_adapter *adapt, enum spatial_partition_mode *mode);
int amdgv_mcp_get_num_xcc_per_partition(struct amdgv_adapter *adapt, uint32_t *count);
int amdgv_mcp_get_xcc_mask_by_idx_part(struct amdgv_adapter *adapt, uint32_t idx_part);
int amdgv_mcp_get_vf_mask_by_xcc(struct amdgv_adapter *adapt, uint32_t idx_xcc);
int amdgv_mcp_get_vf_mask_by_aid(struct amdgv_adapter *adapt, uint32_t idx_aid);

#endif