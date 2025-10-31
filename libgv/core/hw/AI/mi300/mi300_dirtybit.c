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
#include "amdgv.h"
#include "amdgv_device.h"
#include "mi300_dirtybit.h"
#include "gfx_v9_4_3.h"
#include "mmhub_v1_8.h"
#include "sdma_v4_4_2.h"

static const uint32_t this_block = AMDGV_GFX_BLOCK;

#define MI300_DIRTYBIT_BUFFER_SIZE KBYTES_TO_BYTES(16)

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

static inline uint64_t mi300_dirtybit_get_total_bitmap_size(struct amdgv_adapter *adapt, uint64_t fb_size, uint32_t page_size)
{
	uint64_t bitmap_size = -1;

	bitmap_size = amdgv_fb_size_to_bitmap_size_align(fb_size, page_size);
	bitmap_size *= adapt->mcp.num_dagb + (adapt->mcp.gfx.num_xcc / adapt->mcp.num_aid);
	bitmap_size *= adapt->mcp.num_aid;

	return bitmap_size;
}

static uint64_t mi300_dirtybit_get_usable_fb_bitmap_size(struct amdgv_adapter *adapt, uint32_t page_size)
{
	uint64_t fb_size = 0;
	uint32_t fb_size_mb = 0;

	amdgv_gpuiov_get_usable_fb_size(adapt, &fb_size_mb);
	fb_size = MBYTES_TO_BYTES(fb_size_mb);

	return mi300_dirtybit_get_total_bitmap_size(adapt, fb_size, page_size);
}

static int mi300_dirtybit_query_data_sdma(struct amdgv_adapter *adapt, uint64_t mc_addr, struct amdgv_query_dirty_bit_data *data)
{
	int page_size = 0;
	struct amdgv_ring *ring;
	int ring_index = -1;
	uint32_t seq[16] = {0};
	uint32_t *query_bitmap_vaddr = (uint32_t *)data->dbit_plane_data_buffer;
	uint32_t query_bitmap_size;
	uint32_t query_bitmap_size_total;
	void *bitmap_vaddr;
	uint64_t bitmap_gpu_addr;
	uint64_t nr_pages;
	int aid, dagb, xcc_id, ea_per_aid;
	int i, ret = 0;

	if (adapt->sdma.query_dirtybit == NULL) {
		AMDGV_ERROR("query_dirtybit is not set\n");
		return AMDGV_FAILURE;
	}

	if (amdgv_dirtybit_get_dirty_page_size(adapt, &page_size)) {
		AMDGV_ERROR("failed to get dirty page size\n");
		return AMDGV_FAILURE;
	}

	if (adapt->sdma.bitmap_mem == NULL) {
		if (amdgv_sdma_alloc_bitmap_mem(adapt,
				mi300_dirtybit_get_usable_fb_bitmap_size(adapt, page_size))) {
			AMDGV_ERROR("failed to allocate bitmap memory\n");
			return AMDGV_FAILURE;
		}
	}

	bitmap_vaddr = amdgv_memmgr_get_cpu_addr(adapt->sdma.bitmap_mem);
	bitmap_gpu_addr = amdgv_memmgr_get_gpu_addr(adapt->sdma.bitmap_mem);
	if (bitmap_vaddr == NULL) {
		AMDGV_ERROR("failed to get bitmap memory\n");
		return AMDGV_FAILURE;
	}

	nr_pages = DIV_ROUND_UP(data->query_size, page_size);
	nr_pages = nr_pages == 0 ? 1 : nr_pages;
	query_bitmap_size = amdgv_fb_size_to_bitmap_size_align(data->query_size, page_size);
	ea_per_aid = adapt->mcp.num_dagb + (adapt->mcp.gfx.num_xcc / adapt->mcp.num_aid);
	query_bitmap_size_total = mi300_dirtybit_get_total_bitmap_size(adapt, data->query_size, page_size);
	if (query_bitmap_size > data->dbit_plane_data_size) {
		AMDGV_ERROR("dbit_plane_data_size is not enough\n");
		return AMDGV_FAILURE;
	}

	if (query_bitmap_size_total > adapt->sdma.bitmap_size) {
		AMDGV_ERROR("queried fb size is too large\n");
		return AMDGV_FAILURE;
	}

	oss_memset(bitmap_vaddr, 0, query_bitmap_size_total);

	for (xcc_id = 0; xcc_id < adapt->mcp.gfx.num_xcc; xcc_id++) {
		aid = GET_INST(GC, xcc_id) / 2;
		/* Get the SDMA ring in aid #N */
		ring = amdgv_sdma_get_pf_dedicated_ring(adapt, aid, ++ring_index);
		if (ring == NULL) {
			AMDGV_ERROR("failed to get ring at aid=%d, index=%d\n", aid, ring_index);
			return AMDGV_FAILURE;
		}

		/* Submit the SDMA pkg to query dirty bit in GFXHUB in aid #N */
		amdgv_sdma_ring_query_dirtybit(ring, mc_addr, nr_pages,
			bitmap_gpu_addr + ea_per_aid * aid * query_bitmap_size,
			aid, GET_INST(GC, xcc_id) & 0x1, 0,
			!data->dbit_preserve);
		amdgv_fence_emit_polling(ring, &seq[ring_index], SDMA_MAX_TIMEOUT);
		amdgv_ring_commit(ring);

		 /* Get the SDMA ring in aid #N */
		ring = amdgv_sdma_get_pf_dedicated_ring(adapt, aid, ++ring_index);
		if (ring == NULL) {
			AMDGV_ERROR("failed to get ring at aid=%d, index=%d\n", aid, ring_index);
			return AMDGV_FAILURE;
		}
		/* Submit the SDMA pkg to query dirty bit in all the MMHUB instances in aid #N */
		for (dagb = 0; dagb < adapt->mcp.num_dagb; dagb++) {
			amdgv_sdma_ring_query_dirtybit(ring, mc_addr, nr_pages,
				bitmap_gpu_addr + query_bitmap_size * (ea_per_aid * aid + dagb + 1),
				aid, 2, dagb, !data->dbit_preserve);
		}

		amdgv_fence_emit_polling(ring, &seq[ring_index], SDMA_MAX_TIMEOUT);
		amdgv_ring_commit(ring);
	}

	ring_index = -1;
	for (xcc_id = 0; xcc_id < adapt->mcp.gfx.num_xcc; xcc_id++) {
		int r;

		aid = GET_INST(GC, xcc_id) / 2;
		ring = amdgv_sdma_get_pf_dedicated_ring(adapt, aid, ++ring_index);
		/* Wait for the GFXHUB query is done in aid #N */
		r = amdgv_fence_wait_polling(ring, seq[ring_index], SDMA_MAX_TIMEOUT);
		if (r <= 0) {
			AMDGV_ERROR("gc ring wait polling failed, aid=%d, index=%d\n", aid, ring_index);
			ret = AMDGV_FAILURE;
			goto out;
		}

		ring = amdgv_sdma_get_pf_dedicated_ring(adapt, aid, ++ring_index);
		/* Wait for the MMHUB query is done in aid #N */
		r = amdgv_fence_wait_polling(ring, seq[ring_index], SDMA_MAX_TIMEOUT);
		if (r <= 0) {
			AMDGV_ERROR("mm ring wait polling failed, aid=%d, index=%d\n", aid, ring_index);
			ret = AMDGV_FAILURE;
			goto out;
		}
	}

	for (i = 0; i < query_bitmap_size_total; i += query_bitmap_size) {
		uint32_t *ptr;
		int j;

		ptr = (uint32_t *)(((uint8_t *)bitmap_vaddr) + i);
		for (j = 0; j < query_bitmap_size / sizeof(uint32_t); j++) {
			if (ptr[j] != 0)
				query_bitmap_vaddr[j] |= ptr[j];
		}
	}
	ret = 0;
out:
	return ret;
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

	return mi300_dirtybit_query_data_sdma(adapt, mc_addr, data);
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

	adapt->dirtybit.acc_bits_whole_fb = oss_malloc(MI300_DIRTYBIT_BUFFER_SIZE);
	if (adapt->dirtybit.acc_bits_whole_fb == NULL) {
		AMDGV_ERROR("failed to allocate acc_bits_whole_fb\n");
		return AMDGV_FAILURE;
	}
	oss_memset(adapt->dirtybit.acc_bits_whole_fb, 0, MI300_DIRTYBIT_BUFFER_SIZE);

	return 0;
}

static int mi300_dirtybit_sw_fini(struct amdgv_adapter *adapt)
{
	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION))
		return 0;

	oss_free(adapt->dirtybit.acc_bits_whole_fb);
	adapt->dirtybit.acc_bits_whole_fb = NULL;

	return 0;
}

static int mi300_dirtybit_hw_init(struct amdgv_adapter *adapt)
{
	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION))
		return 0;

	mi300_dirtybit_control(adapt, true);

	if (amdgv_dirtybit_assgin_acc_bits_to_vf(adapt)) {
		AMDGV_ERROR("Failed to assign the acc_bits to VF");
		return AMDGV_FAILURE;
	}

	return 0;
}

static int mi300_dirtybit_hw_fini(struct amdgv_adapter *adapt)
{
	if (!(adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION))
		return 0;

	mi300_dirtybit_control(adapt, false);
	amdgv_dirtybit_destroy_vf_acc_bits(adapt);

	return 0;
}

struct amdgv_init_func mi300_dirtybit_func = {
	.name = "mi300_dirtybit_func",
	.sw_init = mi300_dirtybit_sw_init,
	.sw_fini = mi300_dirtybit_sw_fini,
	.hw_init = mi300_dirtybit_hw_init,
	.hw_fini = mi300_dirtybit_hw_fini,
};
