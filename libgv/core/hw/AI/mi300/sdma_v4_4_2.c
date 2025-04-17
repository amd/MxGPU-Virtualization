/*
 * Copyright 2022 Advanced Micro Devices, Inc.
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
#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_misc.h"
#include "amdgv_nbio.h"
#include "amdgv_gfx.h"
#include "mi300/SDMA/sdma_4_4_2_offset.h"
#include "mi300/SDMA/sdma_4_4_2_sh_mask.h"
#include "mi300/GC/gc_9_4_3_offset.h"
#include "amdgv_ras.h"
#include "amdgv_sdma.h"
#include "sdma_v4_4_2.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static void sdma_v4_4_2_query_ras_error_count(struct amdgv_adapter *adapt,
					      void *ras_error_status)
{
	adapt->mca.funcs->pop_block_error_count(adapt,
						AMDGV_RAS_BLOCK__SDMA,
						ras_error_status);
};

const struct amdgv_sdma_ras_funcs sdma_v4_4_2_ras_funcs = {
	.err_cnt_init = NULL,
	.query_ras_error_count = sdma_v4_4_2_query_ras_error_count,
	.reset_ras_error_count = NULL,
};

void sdma_v4_4_2_set_ras_funcs(struct amdgv_adapter *adapt)
{
	adapt->sdma.num_inst_per_aid = SDMA_INST_NUM_PER_AID;
	adapt->sdma.funcs = &sdma_v4_4_2_ras_funcs;
}
static void sdma_v4_4_2_ring_submit_frame(struct amdgv_ring *ring, uint8_t *frame_data)
{
	struct amdgv_adapter *adapt = ring->adapt;
	uint32_t dword_wptr = ring->wptr % ring->ring_size;
	uint32_t *data32 = (uint32_t *)frame_data;
	uint32_t i;
	uint64_t ring_byte_wptr;

	for (i = 0; i < ring->max_dw; i++) {
		ring->ring[dword_wptr++] = data32[i];
	}
	ring->wptr += ring->max_dw;

	// "ring->wptr" is DWORD offset, but register SDMA WPTR is BYTE offset
	ring_byte_wptr = ring->wptr << 2;
	*((volatile uint64_t *)(ring->wptr_cpu_addr)) = ring_byte_wptr;

	if (ring->use_doorbell) {
		WDOORBELL64(ring->doorbell_index, ring_byte_wptr);
	} else {
		WREG32_SOC15(SDMA0, GET_INST(SDMA0, ring->queue), regSDMA_GFX_RB_WPTR, (uint32_t)(ring_byte_wptr));
		WREG32_SOC15(SDMA0, GET_INST(SDMA0, ring->queue), regSDMA_GFX_RB_WPTR_HI, (uint32_t)(ring_byte_wptr >> 32));
	}
}

static const struct amdgv_ring_funcs sdma_ring_funcs = {
	.type = AMDGV_RING_TYPE_SDMA,
	.align_mask = 0xff,
	.nop = 0,
	.support_64bit_ptrs = true,
	.vmhub = 0,
//	.set_wptr = sdma_ring_set_wptr,
	.submit_frame = sdma_v4_4_2_ring_submit_frame,
};

static void sdma_set_ring_funcs(struct amdgv_adapter *adapt)
{
	adapt->sdma.sdma_ring[0].funcs = &sdma_ring_funcs;
	//adapt->sdma.sdma_ring[1].funcs = &sdma_ring_funcs;
}

static int mi300_sdma_ring_init(struct amdgv_adapter *adapt, int ring_id)
{
	struct amdgv_ring *ring = &adapt->sdma.sdma_ring[ring_id];

	ring->me = 0xFFFFFFFF;
	ring->pipe = 0;
	ring->queue = ring_id; // it contains the logical instance

	ring->ring_obj = NULL;
	ring->use_doorbell = true;

	// doorbell_index is DWORD index, and sdma_engine[ring_id] is QWORD index
	ring->doorbell_index = (adapt->doorbell_index.sdma_engine[ring_id]) << 1;
	oss_vsnprintf(ring->name, 12, "sdma%d", ring_id);

	sdma_set_ring_funcs(adapt);
	return amdgv_ring_init(adapt, ring, adapt->opt.paging_queue_frame_bytes_size / sizeof(uint32_t),
				 adapt->opt.paging_queue_frame_number, AMDGV_RING_PRIO_DEFAULT, NULL,
				 MEM_SDMA0_RING + ring_id);
}

static int mi300_sdma_sw_init(struct amdgv_adapter *adapt)
{
	if (!(adapt->flags & AMDGV_FLAG_DISABLE_SDMA_ENGINE)) {
		adapt->sdma.num_sdma_rings = 1;
		mi300_sdma_ring_init(adapt, 0);
	}
	return 0;
}

static int mi300_sdma_sw_fini(struct amdgv_adapter *adapt)
{
	uint32_t i;

	if (!(adapt->flags & AMDGV_FLAG_DISABLE_SDMA_ENGINE)) {
		for (i = 0; i < adapt->sdma.num_sdma_rings; i++) {
			amdgv_ring_fini(&adapt->sdma.sdma_ring[i]);
		}
	}
	return 0;
}

static int mi300_enable_sdma(struct amdgv_adapter *adapt, uint32_t instance)
{
	struct amdgv_ring *ring = &adapt->sdma.sdma_ring[instance];
	uint32_t data;
	uint64_t data64;

	/* 1. Disable IB/RB buffer */
	data = RREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_IB_CNTL);
	data = REG_SET_FIELD(data, SDMA_GFX_IB_CNTL, IB_ENABLE, 0);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_IB_CNTL, data);

	data = RREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_CNTL);
	data = REG_SET_FIELD(data, SDMA_GFX_RB_CNTL, RB_ENABLE, 0);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_CNTL, data);

	/* 2. Set ring buffer base */
	data64 = ring->gpu_addr >> 8;
	data = (uint32_t)data64;
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_BASE, data);
	data = (uint32_t)(data64 >> 32);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_BASE_HI, data);

	/* 3. Set ring buffer size */
	data = RREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_CNTL);
	/* size in log2 of DWORD size = "log2_ring_size" */
	data = REG_SET_FIELD(data, SDMA_GFX_RB_CNTL, RB_SIZE, ring->log2_ring_size);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_CNTL, data);

	/* 4. Reset RB registers */
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_RPTR, 0);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_RPTR_HI, 0);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_WPTR, 0);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_WPTR_HI, 0);

	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_RPTR_ADDR_LO, 0);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_RPTR_ADDR_HI, 0);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_WPTR_POLL_ADDR_LO, 0);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_WPTR_POLL_ADDR_HI, 0);

	/* 5. Disable Poll */
	data = RREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_WPTR_POLL_CNTL);
	data = REG_SET_FIELD(data, SDMA_GFX_RB_WPTR_POLL_CNTL, ENABLE, 0);
	data = REG_SET_FIELD(data, SDMA_GFX_RB_WPTR_POLL_CNTL, F32_POLL_ENABLE, 0);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_WPTR_POLL_CNTL, data);

	/* 6. Enable IB */
	data = RREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_IB_CNTL);
	data = REG_SET_FIELD(data, SDMA_GFX_IB_CNTL, IB_ENABLE, 1);
	data = REG_SET_FIELD(data, SDMA_GFX_IB_CNTL, IB_SWAP_ENABLE, 0);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_IB_CNTL, data);

	/* 7. Set up WPTR poll memory */
	data = (uint32_t)(ring->wptr_gpu_addr);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_WPTR_POLL_ADDR_LO, data);
	data = (uint32_t)(ring->wptr_gpu_addr >> 32);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_WPTR_POLL_ADDR_HI, data);

	data = RREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_WPTR_POLL_CNTL);
	data = REG_SET_FIELD(data, SDMA_GFX_RB_WPTR_POLL_CNTL, ENABLE, 1);
	data = REG_SET_FIELD(data, SDMA_GFX_RB_WPTR_POLL_CNTL, F32_POLL_ENABLE, 1);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_WPTR_POLL_CNTL, data);

	/* 8. Enable doorbell */
	if (ring->use_doorbell) {
		data = RREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_DOORBELL_OFFSET);
		// OFFSET is DWORD aligned, and doorbell is QWORD
		data = REG_SET_FIELD(data, SDMA_GFX_DOORBELL_OFFSET, OFFSET, ring->doorbell_index);
		WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_DOORBELL_OFFSET, data);

		data = RREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_DOORBELL);
		data = REG_SET_FIELD(data, SDMA_GFX_DOORBELL, ENABLE, 1);
		WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_DOORBELL, data);
	}
	/* 9. Unfreeze engine */
	data = RREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_FREEZE);
	data = REG_SET_FIELD(data, SDMA_FREEZE, FREEZE, 0);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_FREEZE, data);

	/* 10. Unhalt engine */
	data = RREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_F32_CNTL);
	data = REG_SET_FIELD(data, SDMA_F32_CNTL, HALT, 0);
	data = REG_SET_FIELD(data, SDMA_F32_CNTL, RESET, 0);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_F32_CNTL, data);

	/* 11. Enable ring buffer */
	data = RREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_CNTL);
	data = REG_SET_FIELD(data, SDMA_GFX_RB_CNTL, RB_ENABLE, 1);
	data = REG_SET_FIELD(data, SDMA_GFX_RB_CNTL, RB_PRIV, 1);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_CNTL, data);

	return 0;
}

static int mi300_disable_sdma(struct amdgv_adapter *adapt, uint32_t instance)
{
	uint32_t data;
	/* 1. Disable IB */
	data = RREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_IB_CNTL);
	data = REG_SET_FIELD(data, SDMA_GFX_IB_CNTL, IB_ENABLE, 0);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_IB_CNTL, data);

	/* 2. Disable ring buffer */
	data = RREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_CNTL);
	data = REG_SET_FIELD(data, SDMA_GFX_RB_CNTL, RB_ENABLE, 0);
	WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_GFX_RB_CNTL, data);

	return 0;
}

static int mi300_sdma_hw_init_internal_set(struct amdgv_adapter *adapt)
{
	struct amdgv_ring *ring = &adapt->sdma.sdma_ring[0];

	return amdgv_ring_init_set(adapt, ring);
}

static int mi300_sdma_hw_init(struct amdgv_adapter *adapt)
{
	uint32_t i;
	struct amdgv_ring *ring;

	if (!(adapt->flags & AMDGV_FLAG_DISABLE_SDMA_ENGINE)) {
		if (!(adapt->flags & AMDGV_FLAG_USE_PF))
			amdgv_sched_context_load(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX);
		mi300_sdma_hw_init_internal_set(adapt);
		for (i = 0; i < adapt->sdma.num_sdma_rings; i++) {
			ring = &adapt->sdma.sdma_ring[i];
			if (in_whole_gpu_reset()) {
				// Clear the ring
				amdgv_ring_clear_ring(ring);
				ring->wptr = 0;
			}
			mi300_enable_sdma(adapt, i);
		}
		if (!(adapt->flags & AMDGV_FLAG_USE_PF))
			amdgv_sched_context_save(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX);
	}
	return 0;
}

static int mi300_sdma_hw_fini(struct amdgv_adapter *adapt)
{
	uint32_t i;

	if (!(adapt->flags & AMDGV_FLAG_DISABLE_SDMA_ENGINE)) {
		for (i = 0; i < adapt->sdma.num_sdma_rings; i++) {
			mi300_disable_sdma(adapt, i);
		}
	}
	return 0;
}

struct amdgv_init_func mi300_sdma_v4_4_2_func = {
	.name = "mi300_sdma_func",
	.sw_init = mi300_sdma_sw_init,
	.sw_fini = mi300_sdma_sw_fini,
	.hw_engine_init = mi300_sdma_hw_init,
	.hw_engine_fini = mi300_sdma_hw_fini,
	.hw_live_init = mi300_sdma_hw_init_internal_set,
};
