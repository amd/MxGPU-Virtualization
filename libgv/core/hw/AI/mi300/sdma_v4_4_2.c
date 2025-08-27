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
#include "mi300_nbio.h"
#include "mi300_dirtybit.h"

static const uint32_t this_block = AMDGV_MEMORY_BLOCK;

static int sdma_v4_4_2_get_dbit_page_size_config(struct amdgv_adapter *adapt)
{
	uint32_t pg_config = 0;

	switch (adapt->dirtybit.mam_adram_mode) {
	case MI300_MAM_ADRAM_MODE_256KB:
		pg_config = SDMA_V4_4_2_DBIT_PAGE_SIZE_CONFIG_256KB;
		break;
	case MI300_MAM_ADRAM_MODE_512KB:
		pg_config = SDMA_V4_4_2_DBIT_PAGE_SIZE_CONFIG_512KB;
		break;
	case MI300_MAM_ADRAM_MODE_1MB:
		pg_config = SDMA_V4_4_2_DBIT_PAGE_SIZE_CONFIG_1MB;
		break;
	case MI300_MAM_ADRAM_MODE_2MB:
		pg_config = SDMA_V4_4_2_DBIT_PAGE_SIZE_CONFIG_2MB;
		break;
	default:
		AMDGV_WARN("Invalid mam_adram_mode: %d, set page size config to default:0\n", adapt->dirtybit.mam_adram_mode);
		pg_config = 0;
		break;
	}

	return pg_config;
}

static void sdma_v4_4_2_ring_query_dirtybit(struct amdgv_ring *ring,
			uint64_t src_addr, uint32_t page_nr,
			uint64_t dst_addr,
			uint32_t aid_inst, uint32_t ea_inst, uint32_t dagb,
			bool clear_dbit)
{
	amdgv_ring_alloc(ring, 5);
	amdgv_ring_write(ring, SDMA_PKT_HEADER_OP(SDMA_OP_POLL_REGMEM) |
		SDMA_PKT_HEADER_SUB_OP(SDMA_SUBOP_POLL_DBIT_WRITE_MEM) |
		SDMA_PKT_HEADER_EA(ea_inst) |
		SDMA_PKT_HEADER_AID(aid_inst) |
		SDMA_PKT_HEADER_DAGB(dagb) |
		SDMA_PKT_HEADER_DBIT_P(clear_dbit));
	amdgv_ring_write(ring, lower_32_bits(dst_addr));
	amdgv_ring_write(ring, upper_32_bits(dst_addr));
	amdgv_ring_write(ring, SDMA_PKT_DBIT_FB_ADDRESS(src_addr));
	amdgv_ring_write(ring, page_nr);
}

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

static int sdma_v4_4_2_sdma_copy(struct amdgv_ring *ring, uint64_t src, uint64_t size, uint64_t dest)
{
	struct amdgv_adapter *adapt = ring->adapt;
	int r = 0;
	uint32_t seq;

	r = amdgv_ring_alloc(ring, 8);

	amdgv_ring_write(ring, SDMA_PKT_HEADER_OP(SDMA_OP_COPY) |
		SDMA_PKT_HEADER_SUB_OP(SDMA_SUBOP_COPY_LINEAR));
	amdgv_ring_write(ring, size - 1);
	amdgv_ring_write(ring, 0);
	amdgv_ring_write(ring, lower_32_bits(src));
	amdgv_ring_write(ring, upper_32_bits(src));
	amdgv_ring_write(ring, lower_32_bits(dest));
	amdgv_ring_write(ring, upper_32_bits(dest));
	amdgv_ring_write(ring, ring->funcs->nop);
	amdgv_fence_emit_polling(ring, &seq, SDMA_MAX_TIMEOUT);
	amdgv_ring_commit(ring);

	r = amdgv_fence_wait_polling(ring, seq, SDMA_MAX_TIMEOUT);
	if (r <= 0) {
		AMDGV_ERROR("%s fb copy failed when polling, seq=%d\n", ring->name, seq);
		return AMDGV_FAILURE;
	}

	return 0;
}

static int sdma_v4_4_2_ring_test_ring(struct amdgv_ring *ring)
{
	return 0;
}

static uint64_t sdma_v4_4_2_ring_get_rptr(struct amdgv_ring *ring)
{
	uint64_t *rptr;

	rptr = (uint64_t *)ring->rptr_cpu_addr;

	return ((*rptr) >> 2);
}

static uint64_t sdma_v4_4_2_ring_get_wptr(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;
	uint64_t wptr;

	if (ring->use_doorbell) {
		wptr = (*((volatile uint64_t *)ring->wptr_cpu_addr));
	} else {
		wptr = RREG32_SOC15(SDMA0, GET_INST(SDMA0, ring->queue), regSDMA_GFX_RB_WPTR_HI);
		wptr = wptr << 32;
		wptr |= RREG32_SOC15(SDMA0, GET_INST(SDMA0, ring->queue), regSDMA_GFX_RB_WPTR);
	}

	return wptr >> 2;
}

static void sdma_v4_4_2_ring_set_wptr(struct amdgv_ring *ring)
{
	struct amdgv_adapter *adapt = ring->adapt;

	if (ring->use_doorbell) {
		adapt->wb.wb[ring->wptr_offs] = ring->wptr;
		WDOORBELL64(ring->doorbell_index, ring->wptr << 2);
	} else {
		WREG32_SOC15(SDMA0, GET_INST(SDMA0, ring->queue), regSDMA_GFX_RB_WPTR, lower_32_bits(ring->wptr << 2));
		WREG32_SOC15(SDMA0, GET_INST(SDMA0, ring->queue), regSDMA_GFX_RB_WPTR_HI, upper_32_bits(ring->wptr << 2));
	}
}

static void sdma_v4_4_2_ring_insert_nop(struct amdgv_ring *ring, uint32_t count)
{
	int i = 0;

	for (i = 0; i < count; i++)
		amdgv_ring_write(ring, SDMA_PKT_NOP_HEADER_OP(0));
}

static void sdma_v4_4_2_ring_emit_fence(struct amdgv_ring *ring, uint64_t addr, uint64_t seq,
				      unsigned flags)
{
	struct amdgv_adapter *adapt = ring->adapt;
	bool write64bit = flags & AMDGV_FENCE_FLAG_64BIT;

	/* write the fence */
	amdgv_ring_write(ring, SDMA_PKT_HEADER_OP(SDMA_OP_FENCE) |
			  SDMA_PKT_FENCE_HEADER_MTYPE(0x3));

	/* zero in first two bits */
	if (addr & 0x3) {
		AMDGV_WARN("addr:%llx\n", addr);
	}
	amdgv_ring_write(ring, lower_32_bits(addr));
	amdgv_ring_write(ring, upper_32_bits(addr));
	amdgv_ring_write(ring, lower_32_bits(seq));

	/* optionally write high bits as well */
	if (write64bit) {
		addr += 4;
		amdgv_ring_write(ring, SDMA_PKT_HEADER_OP(SDMA_OP_FENCE) |
				  SDMA_PKT_FENCE_HEADER_MTYPE(0x3));
		amdgv_ring_write(ring, lower_32_bits(addr));
		amdgv_ring_write(ring, upper_32_bits(addr));
		amdgv_ring_write(ring, upper_32_bits(seq));
	}
}

static const struct amdgv_ring_funcs sdma_ring_funcs = {
	.type = AMDGV_RING_TYPE_SDMA,
	.align_mask = 0xff,
	.nop = SDMA_PKT_NOP_HEADER_OP(SDMA_OP_NOP),
	.support_64bit_ptrs = true,
	.vmhub = VM_MMHUB0,
	.get_rptr = sdma_v4_4_2_ring_get_rptr,
	.get_wptr = sdma_v4_4_2_ring_get_wptr,
	.set_wptr = sdma_v4_4_2_ring_set_wptr,
	.insert_nop = sdma_v4_4_2_ring_insert_nop,
	.emit_fence = sdma_v4_4_2_ring_emit_fence,
	.test_ring = sdma_v4_4_2_ring_test_ring,
	.submit_frame = sdma_v4_4_2_ring_submit_frame,
};

static int mi300_sdma_ring_init(struct amdgv_adapter *adapt, int ring_id, enum amdgv_ring_shared_type shared_type)
{
	struct amdgv_ring *ring = &adapt->sdma.sdma_ring[ring_id];

	ring->me = 0xFFFFFFFF;
	ring->pipe = 0;
	ring->queue = ring_id; // it contains the logical instance

	ring->ring_obj = NULL;
	ring->use_doorbell = true;
	ring->shared_type = shared_type;

	// doorbell_index is DWORD index, and sdma_engine[ring_id] is QWORD index
	ring->doorbell_index = (adapt->doorbell_index.sdma_engine[ring_id]) << 1;
	oss_vsnprintf(ring->name, 12, "sdma%d", ring_id);

	adapt->sdma.sdma_ring[ring_id].funcs = &sdma_ring_funcs;
	return amdgv_ring_init(adapt, ring, adapt->opt.paging_queue_frame_bytes_size / sizeof(uint32_t),
				 adapt->opt.paging_queue_frame_number, AMDGV_RING_PRIO_DEFAULT, NULL,
				 MEM_SDMA0_RING + ring_id);
}

static int mi300_sdma_sw_init(struct amdgv_adapter *adapt)
{
	uint32_t inst;

	adapt->sdma.num_sdma_rings = 0;
	adapt->sdma.num_pf_dedicated_inst = 0;

	if (!(adapt->flags & AMDGV_FLAG_DISABLE_SDMA_ENGINE)) {
		adapt->sdma.num_sdma_rings = 1;
	}

	if ((adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION) && (adapt->asic_type == CHIP_MI308X)) {
		adapt->sdma.num_pf_dedicated_inst = adapt->sdma.harvest_instances;
		adapt->sdma.page_size_config = sdma_v4_4_2_get_dbit_page_size_config(adapt);
	}

	adapt->sdma.sdma_copy = sdma_v4_4_2_sdma_copy;
	adapt->sdma.query_dirtybit = sdma_v4_4_2_ring_query_dirtybit;

	for_each_pfvf_shared_sdma_inst (inst, adapt) {
		mi300_sdma_ring_init(adapt, inst, AMDGV_RING_PFVF_SHARED);
	}

	for_each_pf_dedicated_sdma_inst (inst, adapt) {
		mi300_sdma_ring_init(adapt, inst, AMDGV_RING_PF_DEDICATED);
	}

	return 0;
}

static int mi300_sdma_sw_fini(struct amdgv_adapter *adapt)
{
	int i;

	amdgv_sdma_free_bitmap_mem(adapt);

	for_each_pfvf_shared_sdma_inst (i, adapt) {
		amdgv_ring_fini(&adapt->sdma.sdma_ring[i]);
	}

	for_each_pf_dedicated_sdma_inst (i, adapt) {
		amdgv_ring_fini(&adapt->sdma.sdma_ring[i]);
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

	mi300_nbio_assign_sdma_doorbell(adapt, instance, ring->doorbell_index, 20);

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

	if ((adapt->flags & AMDGV_FLAG_GPUV_LIVE_MIGRATION) && (adapt->asic_type == CHIP_MI308X))
		WREG32_SOC15(SDMA0, GET_INST(SDMA0, instance), regSDMA_HBM_PAGE_CONFIG,
			     adapt->sdma.page_size_config);

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
	uint32_t i;
	int ret = 0;

	for_each_pfvf_shared_sdma_inst (i, adapt) {
		ret = amdgv_ring_init_set(adapt, &adapt->sdma.sdma_ring[i]);
		if (ret)
			return ret;
	}

	for_each_pf_dedicated_sdma_inst (i, adapt) {
		ret = amdgv_ring_init_set(adapt, &adapt->sdma.sdma_ring[i]);
		if (ret)
			return ret;
	}

	return 0;
}

static void mi300_process_sdma_instance(struct amdgv_adapter *adapt, uint32_t ring_id)
{
	struct amdgv_ring *ring;

	ring = &adapt->sdma.sdma_ring[ring_id];
	if (in_whole_gpu_reset()) {
		// Clear the ring
		amdgv_ring_clear_ring(ring);
		ring->wptr = 0;
	}
	mi300_enable_sdma(adapt, ring_id);
}

static void sdma_v4_4_2_harvest_sdma_hw_fini(struct amdgv_adapter *adapt)
{
	int i;

	for_each_pf_dedicated_sdma_inst (i, adapt) {
		mi300_disable_sdma(adapt, i);
	}
}

static void sdma_v4_4_2_pf_sdma_hw_fini(struct amdgv_adapter *adapt)
{
	int i;

	for_each_pfvf_shared_sdma_inst (i, adapt) {
		mi300_disable_sdma(adapt, i);
	}
}

static int sdma_v4_4_2_harvest_sdma_hw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;
	int i;

	for_each_pf_dedicated_sdma_inst (i, adapt) {
		mi300_process_sdma_instance(adapt, i);
	}

	for_each_pf_dedicated_sdma_inst (i, adapt) {
		if (amdgv_ring_test_helper(&adapt->sdma.sdma_ring[i])) {
			ret = AMDGV_FAILURE;
			break;
		}
	}

	if (ret == AMDGV_FAILURE) {
		sdma_v4_4_2_harvest_sdma_hw_fini(adapt);
	}

	return ret;
}

static int sdma_v4_4_2_pf_sdma_hw_init(struct amdgv_adapter *adapt)
{
	int ret = 0;
	int i;

	if (adapt->flags & AMDGV_FLAG_USE_PF)
		amdgv_sched_context_load(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX);

	for_each_pfvf_shared_sdma_inst (i, adapt) {
		mi300_process_sdma_instance(adapt, i);
	}

	for_each_pfvf_shared_sdma_inst (i, adapt) {
		if (amdgv_ring_test_helper(&adapt->sdma.sdma_ring[i])) {
			ret = AMDGV_FAILURE;
			break;
		}
	}

	if (ret == AMDGV_FAILURE) {
		sdma_v4_4_2_pf_sdma_hw_fini(adapt);
	}

	if (adapt->flags & AMDGV_FLAG_USE_PF)
		amdgv_sched_context_save(adapt, AMDGV_PF_IDX, AMDGV_SCHED_BLOCK_GFX);

	return ret;
}

static int mi300_sdma_hw_init(struct amdgv_adapter *adapt)
{
	if (mi300_sdma_hw_init_internal_set(adapt)) {
		AMDGV_ERROR("SDMA hw init internal set failed\n");
		return AMDGV_FAILURE;
	}

	if (sdma_v4_4_2_harvest_sdma_hw_init(adapt)) {
		AMDGV_ERROR("Harvest SDMA hw init failed\n");
		return AMDGV_FAILURE;
	}

	if (sdma_v4_4_2_pf_sdma_hw_init(adapt)) {
		AMDGV_ERROR("PF SDMA hw init failed\n");
		goto err;
	}

	return 0;
err:
	sdma_v4_4_2_harvest_sdma_hw_fini(adapt);

	return AMDGV_FAILURE;
}

static int mi300_sdma_hw_fini(struct amdgv_adapter *adapt)
{
	sdma_v4_4_2_harvest_sdma_hw_fini(adapt);
	sdma_v4_4_2_pf_sdma_hw_fini(adapt);

	return 0;
}

struct amdgv_init_func mi300_sdma_v4_4_2_func = {
	.name = "mi300_sdma_func",
	.is_engine = true,
	.sw_init = mi300_sdma_sw_init,
	.sw_fini = mi300_sdma_sw_fini,
	.hw_init = mi300_sdma_hw_init,
	.hw_fini = mi300_sdma_hw_fini,
	.hw_live_init = mi300_sdma_hw_init_internal_set,
};
