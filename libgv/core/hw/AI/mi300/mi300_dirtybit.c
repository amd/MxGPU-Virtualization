/*
 * Copyright (c) 2025 Advanced Micro Devices, Inc. All rights reserved.
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
 * THE SOFTWARE.
 */

#include "mi300_dirtybit.h"
#include "gfx_v9_4_3.h"
#include "mmhub_v1_8.h"
#include "sdma_v4_4_2.h"

static const uint32_t this_block = AMDGV_GFX_BLOCK;

static int mi300_dirtybit_control(struct amdgv_adapter *adapt, bool enable)
{
	gfx_v9_4_2_dirtybit_control(adapt, enable);
	mmhub_v1_8_dirtybit_control(adapt, enable);

	return 0;
}

static int mi300_dirtybit_query_dirty_page_size(struct amdgv_adapter *adapt, uint32_t *dirty_page_size)
{
	if (dirty_page_size != NULL) {
		switch (adapt->dirtybit.mam_adram_mode) {
		case MI300_MAM_ADRAM_MODE_256KB:
			*dirty_page_size = 256 * 1024;
			break;
		case MI300_MAM_ADRAM_MODE_512KB:
			*dirty_page_size = 512 * 1024;
			break;
		case MI300_MAM_ADRAM_MODE_1MB:
			*dirty_page_size = 1 * 1024 * 1024;
			break;
		case MI300_MAM_ADRAM_MODE_2MB:
			*dirty_page_size = 2 * 1024 * 1024;
			break;
		default:
			*dirty_page_size = -1;
			return AMDGV_FAILURE;
			break;
		}
	} else {
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mi300_dirtybit_query_data(struct amdgv_adapter *adapt, struct amdgv_query_dirty_bit_data *data)
{
	struct amdgv_vf_device *entry = &adapt->array_vf[data->idx_vf];
	uint64_t mc_addr;
	uint64_t vf_fb_offset, vf_fb_size;

	vf_fb_offset = MBYTES_TO_BYTES(entry->fb_offset);
	vf_fb_size = MBYTES_TO_BYTES(entry->fb_size);
	mc_addr = data->query_fb_offset;
	if (mc_addr > vf_fb_size)
		return AMDGV_FAILURE;

	if (mc_addr + data->query_size > vf_fb_size)
		data->query_size = vf_fb_size - mc_addr;

	mc_addr += adapt->mc_fb_loc_addr + vf_fb_offset;
	if (adapt->xgmi.phy_nodes_num > 1)
		mc_addr += adapt->xgmi.phy_node_id * adapt->xgmi.node_segment_size;

	AMDGV_DEBUG("query_offset=0x%llx, mc_addr=0x%llx, query_size=0x%llx, bm_size=0x%llx,"
		"vf_fb=0x%llx, vf_fb_size=0x%llx"
		"node_id=%d, segmet_size=0x%llx\n",
		data->query_fb_offset, mc_addr, data->query_size, data->dbit_plane_data_size,
		vf_fb_offset, vf_fb_size, adapt->xgmi.phy_node_id, adapt->xgmi.node_segment_size);

	return sdma_v4_4_2_sdma_query_dirtybit(adapt, mc_addr, data);
}

static const struct amdgv_dirtybit_funcs mi300_db_funcs = {
	.control = mi300_dirtybit_control,
	.query_dirty_page_size = mi300_dirtybit_query_dirty_page_size,
	.query_data = mi300_dirtybit_query_data,
};

static int mi300_dirtybit_sw_init(struct amdgv_adapter *adapt)
{
	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION)) {
		adapt->dirtybit.mam_adram_mode = MI300_MAM_ADRAM_MODE_MAX;
		return 0;
	}

	adapt->dirtybit.funcs = &mi300_db_funcs;
	adapt->dirtybit.mam_adram_mode = MI300_MAM_ADRAM_MODE_2MB;

	return 0;
}

static int mi300_dirtybit_sw_fini(struct amdgv_adapter *adapt)
{
	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION))
		return 0;

	return 0;
}

static int mi300_dirtybit_hw_init(struct amdgv_adapter *adapt)
{
	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION))
		return 0;

	mi300_dirtybit_control(adapt, true);

	return 0;
}

static int mi300_dirtybit_hw_fini(struct amdgv_adapter *adapt)
{
	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION))
		return 0;

	mi300_dirtybit_control(adapt, false);

	return 0;
}

struct amdgv_init_func mi300_dirtybit_func = {
	.name = "mi300_dirtybit_func",
	.sw_init = mi300_dirtybit_sw_init,
	.sw_fini = mi300_dirtybit_sw_fini,
	.hw_init = mi300_dirtybit_hw_init,
	.hw_fini = mi300_dirtybit_hw_fini,
};
