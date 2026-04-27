/*
 * Copyright 2025 Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */
#include "amdgv_device.h"
#include "amdgv_sdma.h"

static const uint32_t this_block = AMDGV_SDMA_BLOCK;

struct amdgv_ring *amdgv_sdma_get_available_ring(struct amdgv_adapter *adapt, enum amdgv_ring_shared_type shared_type)
{
	int i;
	struct amdgv_ring *ring = NULL;

	if (shared_type == AMDGV_RING_PF_DEDICATED) {
		for_each_pf_dedicated_sdma_inst (i, adapt) {
			ring = &adapt->sdma.sdma_ring[i];
			break;
		}
	} else if (shared_type == AMDGV_RING_PFVF_SHARED) {
		for_each_pfvf_shared_sdma_inst (i, adapt) {
			ring = &adapt->sdma.sdma_ring[i];
			break;
		}
	}

	return ring;
}

int amdgv_sdma_ring_copy(struct amdgv_ring *ring, uint64_t src, uint64_t size, uint64_t dst)
{
	struct amdgv_adapter *adapt = ring->adapt;
	int ret = 0;

	if (ring && adapt->sdma.sdma_copy) {
		int cur_idx_vf = AMDGV_INVALID_IDX_VF;

		if (ring->shared_type == AMDGV_RING_PFVF_SHARED) {
			amdgv_gpuiov_get_active_vf_idx(adapt, AMDGV_SCHED_BLOCK_GFX, &cur_idx_vf);
			amdgv_sched_context_switch_to_vf(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX);
		}
		ret = adapt->sdma.sdma_copy(ring, src, size, dst);
		if (ring->shared_type == AMDGV_RING_PFVF_SHARED) {
			amdgv_sched_context_switch_to_vf(adapt, cur_idx_vf, AMDGV_SCHED_BLOCK_GFX);
		}
	}

	return ret;
}

/*
 * Get the pf dedicated sdma instance in a special AID.
 *
 * @adapt: the adapter.
 * @aid: which @aid we want to get the instance from.
 * @index: the @index-th instance in one AID.
 *
 * @return: the pf dedicated sdma instance.
 */
struct amdgv_ring *amdgv_sdma_get_pf_dedicated_ring(struct amdgv_adapter *adapt, int aid, int index)
{
	uint32_t instance = AMDGV_SDMA_PF_DECIDATED_RING_START_INDEX(adapt);
	uint32_t rings_per_aid = adapt->sdma.num_pf_dedicated_inst / adapt->mcp.num_aid;

	if (rings_per_aid == 0)
		return NULL;

	instance += aid * rings_per_aid;
	instance += index % rings_per_aid;

	if (instance >= AMDGV_SDMA_PF_DECIDATED_RING_END_INDEX(adapt))
		return NULL;

	return &adapt->sdma.sdma_ring[instance];;
}

struct amdgv_ring *amdgv_sdma_get_pfvf_shared_ring(struct amdgv_adapter *adapt, int aid, int index)
{
	uint32_t instance = 0;

	instance = aid * adapt->sdma.num_inst_per_aid + (index % adapt->sdma.num_inst_per_aid);
	if (instance >= adapt->sdma.num_instances)
		return NULL;

	return &adapt->sdma.sdma_ring[instance];
}

int amdgv_sdma_alloc_bitmap_mem(struct amdgv_adapter *adapt, uint64_t bitmap_size)
{
	adapt->sdma.bitmap_mem = amdgv_memmgr_alloc_sys_align(&adapt->memmgr_sys,
					bitmap_size, PAGE_SIZE, 0, NULL);
	if (adapt->sdma.bitmap_mem == NULL) {
		AMDGV_WARN("Failed to allocate dma memory\n");
		return AMDGV_FAILURE;
	}

	adapt->sdma.bitmap_size = bitmap_size;
	return 0;
}

int amdgv_sdma_free_bitmap_mem(struct amdgv_adapter *adapt)
{
	if (adapt->sdma.bitmap_mem) {
		amdgv_memmgr_free(adapt->sdma.bitmap_mem);
		adapt->sdma.bitmap_mem = NULL;
	}
	adapt->sdma.bitmap_size = 0;

	return 0;
}