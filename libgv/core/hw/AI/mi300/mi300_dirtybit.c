/* Copyright Advanced Micro Devices, Inc.
 *
 * SPDX-License-Identifier: MIT
 */

#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_sched_internal.h"
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
	int ret = 0;

	if (dirty_page_size == NULL)
		return AMDGV_FAILURE;

	switch (adapt->asic_type) {
	case CHIP_MI350X:
		*dirty_page_size = MI350_4MB_DIRTY_PAGE_SIZE;
		break;
	case CHIP_MI308X:
		*dirty_page_size = MI308_2MB_DIRTY_PAGE_SIZE;
		break;
	default:
		*dirty_page_size = -1;
		ret = AMDGV_FAILURE;
		break;
	}

	return ret;
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

static struct amdgv_ring* mi300_dirtybit_get_available_ring(struct amdgv_adapter *adapt, int aid, int index)
{
	if (adapt->sdma.num_pf_dedicated_inst != 0) {
		return amdgv_sdma_get_pf_dedicated_ring(adapt, aid, index);
	} else {
		return amdgv_sdma_get_pfvf_shared_ring(adapt, aid, index);
	}
}

static int mi300_dirtybit_query_data_sdma(struct amdgv_adapter *adapt, uint64_t mc_addr, struct amdgv_query_dirty_bit_data *data)
{
	int page_size = 0;
	struct amdgv_ring *ring;
	int ring_index = 0;
	uint32_t seq[16] = {0};
	uint32_t *query_bitmap_vaddr = (uint32_t *)data->dbit_plane_data_buffer;
	uint32_t query_bitmap_size, bitmap_mem_offset;
	uint32_t query_bitmap_size_total;
	void *bitmap_vaddr;
	uint64_t bitmap_gpu_addr;
	uint64_t nr_pages;
	int aid, dagb, xcc_id, xcc_per_aid, ring_index_in_aid;
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

	bitmap_mem_offset = 0;
	nr_pages = DIV_ROUND_UP(data->query_size, page_size);
	nr_pages = nr_pages == 0 ? 1 : nr_pages;
	query_bitmap_size = amdgv_fb_size_to_bitmap_size_align(data->query_size, page_size);
	xcc_per_aid = adapt->mcp.gfx.num_xcc / adapt->mcp.num_aid;
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

	if (!IS_DEDICATED_SDMA_RING_AVAILABLE(adapt))
		amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX);

	/* Submit GFXHUB queries for all xccs */
	for (xcc_id = 0; xcc_id < adapt->mcp.gfx.num_xcc; xcc_id++) {
		/* Calculate which aid this xcc belongs to */
		aid = GET_INST(GC, xcc_id) / 2;
		/* Calculate the available SDMA ring index in aid */
		ring_index_in_aid = xcc_id % xcc_per_aid;
		/* Get the SDMA ring for this xcc */
		ring = mi300_dirtybit_get_available_ring(adapt, aid, ring_index_in_aid);
		if (ring == NULL) {
			AMDGV_ERROR("failed to get ring at xcc=%d, aid=%d, index=%d\n", xcc_id, aid, ring_index);
			return AMDGV_FAILURE;
		}
		/* Submit the SDMA pkg to query dirty bit in GFXHUB for this xcc */
		amdgv_sdma_ring_query_dirtybit(ring, mc_addr, nr_pages,
			bitmap_gpu_addr + bitmap_mem_offset,
			aid, GET_INST(GC, xcc_id) & 0x1, 0,
			!data->dbit_preserve);
		AMDGV_DEBUG("[GFXHUB][AID%d][XCC%d]bitmap_gpu_addr: 0x%llx, size: 0x%llx, ring_index=%d, ring_name=%s\n",
					aid, GET_INST(GC, xcc_id),
					bitmap_gpu_addr + bitmap_mem_offset, query_bitmap_size, ring_index, ring->name);
		bitmap_mem_offset += query_bitmap_size;
		if (bitmap_mem_offset > query_bitmap_size_total) {
			AMDGV_ERROR("Required memory exceeded allocated memory size\n");
			return AMDGV_FAILURE;
		}
		amdgv_fence_emit_polling(ring, &seq[ring_index], SDMA_MAX_TIMEOUT);
		ring_index++;
		amdgv_ring_commit(ring);
	}

	/* Submit MMHUB queries for all aids */
	for (aid = 0; aid < adapt->mcp.num_aid; aid++) {
		/* We've used the first #xcc_per_aid rings in aid for GFXHUB queries
		 * so the next ring index is #xcc_per_aid for MMHUB queries
		 */
		ring_index_in_aid = xcc_per_aid;
		/* Get the SDMA ring in aid #N */
		ring = mi300_dirtybit_get_available_ring(adapt, aid, ring_index_in_aid);
		if (ring == NULL) {
			AMDGV_ERROR("failed to get ring at aid=%d, index=%d\n", aid, ring_index);
			return AMDGV_FAILURE;
		}
		/* Submit the SDMA pkg to query dirty bit in all the MMHUB instances in aid #N */
		for (dagb = 0; dagb < adapt->mcp.num_dagb; dagb++) {
			amdgv_sdma_ring_query_dirtybit(ring, mc_addr, nr_pages,
				bitmap_gpu_addr + bitmap_mem_offset, aid, 2, dagb, !data->dbit_preserve);
			AMDGV_DEBUG("[MMHUB][AID%d][DAGB%d]bitmap_gpu_addr: 0x%llx, size: 0x%llx, ring_index=%d, ring_name=%s\n",
						aid, dagb,
						bitmap_gpu_addr + bitmap_mem_offset, query_bitmap_size, ring_index, ring->name);
			bitmap_mem_offset += query_bitmap_size;
			if (bitmap_mem_offset > query_bitmap_size_total) {
				AMDGV_ERROR("Required memory exceeded allocated memory size\n");
				return AMDGV_FAILURE;
			}
		}

		amdgv_fence_emit_polling(ring, &seq[ring_index], SDMA_MAX_TIMEOUT);
		ring_index++;
		amdgv_ring_commit(ring);
	}

	/* Wait for GFXHUB queries to complete for all xccs */
	ring_index = 0;
	for (xcc_id = 0; xcc_id < adapt->mcp.gfx.num_xcc; xcc_id++) {
		int r;
		/* Calculate which aid this xcc belongs to */
		aid = GET_INST(GC, xcc_id) / 2;
		ring_index_in_aid = xcc_id % xcc_per_aid;
		ring = mi300_dirtybit_get_available_ring(adapt, aid, ring_index_in_aid);
		r = amdgv_fence_wait_polling(ring, seq[ring_index], SDMA_MAX_TIMEOUT);
		ring_index++;
		if (r <= 0) {
			AMDGV_ERROR("gc ring wait polling failed, xcc=%d, aid=%d, index=%d\n", xcc_id, aid, ring_index);
			return AMDGV_FAILURE;
		}
	}

	/* Wait for MMHUB queries to complete for all aids */
	for (aid = 0; aid < adapt->mcp.num_aid; aid++) {
		int r;

		ring_index_in_aid = xcc_per_aid;
		ring = mi300_dirtybit_get_available_ring(adapt, aid, ring_index_in_aid);
		/* Wait for the MMHUB query is done in aid #N */
		r = amdgv_fence_wait_polling(ring, seq[ring_index], SDMA_MAX_TIMEOUT);
		ring_index++;
		if (r <= 0) {
			AMDGV_ERROR("mm ring wait polling failed, aid=%d, index=%d\n", aid, ring_index);
			return AMDGV_FAILURE;
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
		adapt->dirtybit.mam_adram_mode = MAM_ADRAM_MODE_INVALID;
		return 0;
	}

	adapt->dirtybit.funcs = &mi300_db_funcs;
	switch (adapt->asic_type) {
	case CHIP_MI308X:
		adapt->dirtybit.mam_adram_mode = MI308_2MB_MAM_ADRAM_MODE;
		break;
	case CHIP_MI350X:
		adapt->dirtybit.mam_adram_mode = MI350_4MB_MAM_ADRAM_MODE;
		break;
	default:
		AMDGV_WARN("Invalid asic_type: %d, set mam_adram_mode to default:0\n", adapt->asic_type);
		break;
	}

	adapt->dirtybit.acc_bits_whole_fb = oss_malloc(AMDGV_DIRTYBIT_BUFFER_SIZE);
	if (adapt->dirtybit.acc_bits_whole_fb == NULL) {
		AMDGV_ERROR("failed to allocate acc_bits_whole_fb\n");
		return AMDGV_FAILURE;
	}
	oss_memset(adapt->dirtybit.acc_bits_whole_fb, 0, AMDGV_DIRTYBIT_BUFFER_SIZE);

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
