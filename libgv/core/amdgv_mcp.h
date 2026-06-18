/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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
	uint32_t num_dagb;
	enum spatial_partition_mode spatial_partition_mode;
	enum amdgv_accelerator_partition_mode accelerator_partition_mode;
	enum amdgv_memory_partition_mode memory_partition_mode;
	bool mem_mode_switch_requested;
	enum amdgv_cc_mode cc_mode;
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
	int (*get_xcp_by_xcc)(struct amdgv_adapter *adapt, uint32_t idx_xcc);
};
const char *amdgv_mcp_spatial_partition_to_name(enum spatial_partition_mode mode);
int amdgv_mcp_get_num_gfx_spatial_partitions(struct amdgv_adapter *adapt, uint32_t *count);
int amdgv_mcp_get_spatial_partition_mode(struct amdgv_adapter *adapt, enum spatial_partition_mode *mode);
int amdgv_mcp_get_num_xcc_per_partition(struct amdgv_adapter *adapt, uint32_t *count);
int amdgv_mcp_get_xcc_mask_by_idx_part(struct amdgv_adapter *adapt, uint32_t idx_part);
int amdgv_mcp_get_vf_mask_by_xcc(struct amdgv_adapter *adapt, uint32_t idx_xcc);
int amdgv_mcp_get_vf_mask_by_aid(struct amdgv_adapter *adapt, uint32_t idx_aid);
int amdgv_mcp_get_xcp_by_xcc(struct amdgv_adapter *adapt, uint32_t idx_xcc);

#endif