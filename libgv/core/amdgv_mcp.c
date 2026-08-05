/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
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

/* Partition a max_xcc-wide physical XCC range into num_partitions, keeping only
 * the XCCs present in xcc_mask (harvested ones are skipped in place). When
 * num_partitions equals the number of present XCCs the layout is CPX (one XCC
 * per partition, in physical order); otherwise the physical range is sliced
 * into equal ranges. Each XCC pulls in the VCN it aligns to, the max_xcc
 * physical XCCs dividing evenly among max_vcn VCNs.
 */
int amdgv_mcp_build_partition_layout(uint32_t xcc_mask, uint32_t max_xcc,
				     uint32_t max_vcn, uint32_t num_partitions,
				     struct amdgv_spatial_partition_layout *out)
{
	uint32_t num_xcc = 0;
	uint32_t stride, i, phys;

	if (num_partitions == 0 || num_partitions > AMDGV_MCP_MAX_SPATIAL_PARTITIONS ||
	    max_xcc == 0 || max_xcc >= 32 || max_vcn == 0 || max_vcn > max_xcc ||
	    (xcc_mask >> max_xcc))
		return AMDGV_FAILURE;

	for_each_id (phys, xcc_mask)
		num_xcc++;

	if (num_xcc == 0 || num_xcc % num_partitions)
		return AMDGV_FAILURE;

	oss_memset(out, 0, sizeof(*out));
	out->mode = SPATIAL_PARTITION_MODE__UNKNOWN;
	out->num_partitions = num_partitions;

	if (num_partitions == num_xcc) {
		i = 0;
		for_each_id (phys, xcc_mask) {
			out->part[i].xcc_mask = BIT(phys);
			out->part[i].vcn_mask = BIT(phys * max_vcn / max_xcc);
			i++;
		}
	} else if (max_xcc % num_partitions == 0) {
		stride = max_xcc / num_partitions;
		for (i = 0; i < num_partitions; i++) {
			for (phys = i * stride; phys < (i + 1) * stride; phys++) {
				if (!(xcc_mask & BIT(phys)))
					continue;
				out->part[i].xcc_mask |= BIT(phys);
				out->part[i].vcn_mask |= BIT(phys * max_vcn / max_xcc);
			}
			/* A fully-harvested slice means num_partitions does not fit. */
			if (!out->part[i].xcc_mask)
				return AMDGV_FAILURE;
		}
	} else {
		return AMDGV_FAILURE;
	}

	return 0;
}

/* Build every partitioning the adapter can expose for its current topology into
 * mcp.partition_layouts: SPX (whole chip), the N-way modes that hold more than
 * one XCC (so they stay distinct from CPX) and split evenly, and CPX (one XCC
 * per partition). Called once at init; consumers then read mcp.partition_layouts.
 */
void amdgv_mcp_build_partition_layouts(struct amdgv_adapter *adapt)
{
	struct amdgv_mcp *mcp = &adapt->mcp;
	uint32_t xcc_mask = mcp->gfx.xcc_mask;
	uint32_t max_xcc = mcp->gfx.max_xcc;
	uint32_t max_vcn = adapt->config.mm.count[AMDGV_VCN_ENGINE];
	struct amdgv_spatial_partition_layout *out = mcp->partition_layouts;
	uint32_t num_xcc = 0, phys, n = 0;

	for_each_id (phys, xcc_mask)
		num_xcc++;

	if (!amdgv_mcp_build_partition_layout(xcc_mask, max_xcc, max_vcn, 1, &out[n]))
		out[n++].mode = SPATIAL_PARTITION_MODE__SPX;
	if (num_xcc > 2 &&
	    !amdgv_mcp_build_partition_layout(xcc_mask, max_xcc, max_vcn, 2, &out[n]))
		out[n++].mode = SPATIAL_PARTITION_MODE__DPX;
	if (num_xcc > 4 &&
	    !amdgv_mcp_build_partition_layout(xcc_mask, max_xcc, max_vcn, 4, &out[n]))
		out[n++].mode = SPATIAL_PARTITION_MODE__QPX;
	if (num_xcc > 1 &&
	    !amdgv_mcp_build_partition_layout(xcc_mask, max_xcc, max_vcn, num_xcc, &out[n]))
		out[n++].mode = SPATIAL_PARTITION_MODE__CPX;

	mcp->num_partition_layouts = n;
}

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

/* Logical XCC */
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

int amdgv_mcp_get_xcp_by_xcc(struct amdgv_adapter *adapt, uint32_t idx_xcc)
{
	if (adapt->mcp.get_xcp_by_xcc) {
		return adapt->mcp.get_xcp_by_xcc(adapt, idx_xcc);
	} else {
		return 0;
	}
}
