/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include <amdgv_device.h>
#include "amdgv_mcp.h"
#include "amdgv_sched.h"

#include "asic_reg/NBIO/nbio_6_3_2_offset.h"
#include "asic_reg/NBIO/nbio_6_3_2_sh_mask.h"


static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;


static int nbio_v6_3_2_mcp_set_spatial_partition_mode(struct amdgv_adapter *adapt)
{
	enum spatial_partition_mode mode = SPATIAL_PARTITION_MODE__UNKNOWN;

	switch (adapt->num_vf) {
	case 6:
	case 8:
		mode = SPATIAL_PARTITION_MODE__CPX;
		break;
	case 4:
		mode = SPATIAL_PARTITION_MODE__QPX;
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
	}

	adapt->mcp.spatial_partition_mode = mode;
	AMDGV_INFO("Spatial Partition mode: %s\n", amdgv_mcp_spatial_partition_to_name(mode));

	return 0;
}

static int nbio_v6_3_2_mcp_set_num_xcc_per_partition(struct amdgv_adapter *adapt)
{
	int num_xcc_per_partition = 0;

	switch (adapt->mcp.spatial_partition_mode) {
	case SPATIAL_PARTITION_MODE__CPX:
		num_xcc_per_partition = 1;
		break;
	case SPATIAL_PARTITION_MODE__QPX:
		num_xcc_per_partition = adapt->mcp.gfx.num_xcc / 4;
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
	}

	adapt->mcp.gfx.num_xcc_per_partition = num_xcc_per_partition;

	return 0;
}

static int nbio_v6_3_2_mcp_get_gfx_num_spatial_partitions(struct amdgv_adapter *adapt,
						    uint32_t *count)
{
	switch (adapt->mcp.spatial_partition_mode) {
	case SPATIAL_PARTITION_MODE__CPX:
		*count = adapt->mcp.gfx.num_xcc;
		break;
	case SPATIAL_PARTITION_MODE__QPX:
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
	}

	return 0;
}

static int nbio_v6_3_2_mcp_get_spatial_partition_mode(struct amdgv_adapter *adapt,
						enum spatial_partition_mode *mode)
{
	switch (adapt->mcp.spatial_partition_mode) {
	case SPATIAL_PARTITION_MODE__CPX:
	case SPATIAL_PARTITION_MODE__QPX:
	case SPATIAL_PARTITION_MODE__DPX:
	case SPATIAL_PARTITION_MODE__SPX:
		*mode = adapt->mcp.spatial_partition_mode;
		break;
	default:
		AMDGV_ERROR("Unknown Spatial Partition mode.");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int nbio_v6_3_2_mcp_get_vf_mask_by_xcc_mask(struct amdgv_adapter *adapt, uint32_t xcc_mask)
{
	uint32_t i = 0;
	uint32_t vf_mask = 0;

	for (i = 0; i < adapt->sched.num_spatial_partitions; i++) {
		if (adapt->sched.spatial_part[i].xcc_mask & xcc_mask) {
			vf_mask |= adapt->sched.spatial_part[i].idx_vf_mask;
		}
	}

	return vf_mask;
}

static int nbio_v6_3_2_mcp_get_vf_mask_by_xcc(struct amdgv_adapter *adapt, uint32_t idx_xcc)
{
	return nbio_v6_3_2_mcp_get_vf_mask_by_xcc_mask(adapt, BIT(idx_xcc));
}

static int nbio_v6_3_2_mcp_get_vf_mask_by_aid(struct amdgv_adapter *adapt, uint32_t idx_aid)
{
	/* 4 XCC per AID */
	return nbio_v6_3_2_mcp_get_vf_mask_by_xcc_mask(adapt,
							  BIT(idx_aid * 4)         |
							  BIT(((idx_aid * 4) + 1)) |
							  BIT(((idx_aid * 4) + 2)) |
							  BIT(((idx_aid * 4) + 3)));
}

static int nbio_v6_3_2_mcp_sw_init(struct amdgv_adapter *adapt)
{
	adapt->mcp.amdgv_mcp_get_gfx_num_spatial_partitions =
		nbio_v6_3_2_mcp_get_gfx_num_spatial_partitions;
	adapt->mcp.amdgv_mcp_get_spatial_partition_mode = nbio_v6_3_2_mcp_get_spatial_partition_mode;
	adapt->mcp.get_vf_mask_by_xcc = nbio_v6_3_2_mcp_get_vf_mask_by_xcc;
	adapt->mcp.get_vf_mask_by_aid = nbio_v6_3_2_mcp_get_vf_mask_by_aid;

	if (nbio_v6_3_2_mcp_set_spatial_partition_mode(adapt))
		return AMDGV_FAILURE;

	if (nbio_v6_3_2_mcp_set_num_xcc_per_partition(adapt))
		return AMDGV_FAILURE;

	amdgv_mcp_build_partition_layouts(adapt);

	return 0;
}

/*
	Partition			Memory NPS	|
				1	2	|
	SPX			X		|
	DPX			X	X	|
	QPX			X	X	|
	CPX			X	X	|
*/
static int nbio_v6_3_2_mcp_check_mem_partition_cap(struct amdgv_adapter *adapt)
{
	int ret = 0;
	uint32_t mem_cap = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_PARTITION_MEM_CAP));

	switch (adapt->mcp.spatial_partition_mode) {
	case SPATIAL_PARTITION_MODE__SPX:
		if (!REG_GET_FIELD(mem_cap, BIF_BX_PF0_PARTITION_MEM_CAP, NPS1_SUPPORT))
			ret = AMDGV_FAILURE;
		break;
	case SPATIAL_PARTITION_MODE__DPX:
	case SPATIAL_PARTITION_MODE__QPX:
	case SPATIAL_PARTITION_MODE__CPX:
		if (!REG_GET_FIELD(mem_cap, BIF_BX_PF0_PARTITION_MEM_CAP, NPS1_SUPPORT) &&
		    !REG_GET_FIELD(mem_cap, BIF_BX_PF0_PARTITION_MEM_CAP, NPS2_SUPPORT))
			ret = AMDGV_FAILURE;
		break;
	default:
		ret = AMDGV_FAILURE;
		break;
	}

	if (ret)
		AMDGV_ERROR("Selected Partition Mode is not supported!\n");

	return ret;
}

static int nbio_v6_3_2_mcp_check_compute_partition_cap(struct amdgv_adapter *adapt)
{
	int ret = 0;
	uint32_t compute_cap = RREG32(SOC15_REG_OFFSET(NBIO, 0, regBIF_BX_PF0_PARTITION_COMPUTE_CAP));

	switch (adapt->mcp.spatial_partition_mode) {
	case SPATIAL_PARTITION_MODE__SPX:
		if (!REG_GET_FIELD(compute_cap, BIF_BX_PF0_PARTITION_COMPUTE_CAP,
				   SPX_SUPPORT))
			ret = AMDGV_FAILURE;
		break;
	case SPATIAL_PARTITION_MODE__DPX:
		if (!REG_GET_FIELD(compute_cap, BIF_BX_PF0_PARTITION_COMPUTE_CAP,
				   DPX_SUPPORT))
			ret = AMDGV_FAILURE;
		break;
	case SPATIAL_PARTITION_MODE__QPX:
		if (!REG_GET_FIELD(compute_cap, BIF_BX_PF0_PARTITION_COMPUTE_CAP,
				   QPX_SUPPORT))
			ret = AMDGV_FAILURE;
		break;
	case SPATIAL_PARTITION_MODE__CPX:
		if (!REG_GET_FIELD(compute_cap, BIF_BX_PF0_PARTITION_COMPUTE_CAP,
				   CPX_SUPPORT))
			ret = AMDGV_FAILURE;
		break;
	default:
		ret = AMDGV_FAILURE;
		break;
	}

	if (ret)
		AMDGV_ERROR("Selected Partition Mode is not supported!\n");

	return ret;
}

static int nbio_v6_3_2_mcp_hw_init(struct amdgv_adapter *adapt)
{
	int r;

	r = nbio_v6_3_2_mcp_check_compute_partition_cap(adapt);
	if (r)
		return r;

	r = nbio_v6_3_2_mcp_check_mem_partition_cap(adapt);
	if (r)
		return r;

	return 0;
}

static int nbio_v6_3_2_mcp_sw_fini(struct amdgv_adapter *adapt)
{
	adapt->mcp.amdgv_mcp_get_gfx_num_spatial_partitions = NULL;
	adapt->mcp.amdgv_mcp_get_spatial_partition_mode = NULL;
	adapt->mcp.get_vf_mask_by_xcc = NULL;
	adapt->mcp.get_vf_mask_by_aid = NULL;

	return 0;
}

static int nbio_v6_3_2_mcp_hw_fini(struct amdgv_adapter *adapt)
{
	return 0;
}

const struct amdgv_init_func nbio_v6_3_2_mcp_func = {
	.name = "nbio_v6_3_2_mcp_func",
	.sw_init = nbio_v6_3_2_mcp_sw_init,
	.sw_fini = nbio_v6_3_2_mcp_sw_fini,
	.hw_init = nbio_v6_3_2_mcp_hw_init,
	.hw_fini = nbio_v6_3_2_mcp_hw_fini,
};
