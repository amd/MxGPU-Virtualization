/*
 * Copyright (C) 2022 Advanced Micro Devices, Inc. All rights reserved.
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


#include "amdgv.h"
#include "amdgv_device.h"
#include "amdgv_misc.h"
#include "amdgv_nbio.h"
#include "amdgv_gfx.h"
#include <navi3/GC/gc_11_0_3_offset.h>
#include <navi3/GC/gc_11_0_3_sh_mask.h>
#include "navi32_sdma.h"

static const uint32_t this_block = AMDGV_COMMUNICATION_BLOCK;

/* set SDMA golden setting for SDMA cntl and UTC L1 */
static const struct amdgv_reg_golden golden_settings_sdma_6_0[] = {
	SOC15_REG_GOLDEN_VALUE(GC, 0, regSDMA0_CNTL, 0xffffffff, 0x10002441),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regSDMA1_CNTL, 0xffffffff, 0x10002441),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regSDMA0_SEM_WAIT_FAIL_TIMER_CNTL, 0xffffffff, 0x00000000),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regSDMA1_SEM_WAIT_FAIL_TIMER_CNTL, 0xffffffff, 0x00000000),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regSDMA0_UTCL1_CNTL, 0xffffffff, 0x2c000689),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regSDMA1_UTCL1_CNTL, 0xffffffff, 0x2c000689),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regSDMA0_UTCL1_PAGE, 0xffffffff, 0x10cec20),
	SOC15_REG_GOLDEN_VALUE(GC, 0, regSDMA1_UTCL1_PAGE, 0xffffffff, 0x10cec20),
};

void navi32_sdma_program_golden_settings(struct amdgv_adapter *adapt)
{
	amdgv_program_register_sequence(adapt, golden_settings_sdma_6_0,
					ARRAY_SIZE(golden_settings_sdma_6_0));
}

static void navi32_sdma_ring_submit_frame(struct amdgv_ring *ring, uint8_t *frame_data)
{
	struct amdgv_adapter *adapt = ring->adapt;
	uint32_t dword_wptr = (ring->wptr % ring->ring_size);
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

	amdgv_misc_hdp_flush(adapt);

	if (ring->use_doorbell) {
		WDOORBELL64(ring->doorbell_index, ring_byte_wptr);
	} else if (ring->me == 0) {
		WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_WPTR, (uint32_t)(ring_byte_wptr));
		WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_WPTR_HI, (uint32_t)(ring_byte_wptr >> 32));
	} else {
		WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_WPTR, (uint32_t)(ring_byte_wptr));
		WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_WPTR_HI, (uint32_t)(ring_byte_wptr >> 32));
	}
}

static const struct amdgv_ring_funcs sdma_ring_funcs = {
	.type = AMDGV_RING_TYPE_SDMA,
	.align_mask = 0xff,
	.nop = 0,
	.support_64bit_ptrs = true,
	.vmhub = 0,
//	.set_wptr = sdma_ring_set_wptr,
	.submit_frame = navi32_sdma_ring_submit_frame,
};


static void sdma_set_ring_funcs(struct amdgv_adapter *adapt)
{
	adapt->sdma.sdma_ring[0].funcs = &sdma_ring_funcs;
	adapt->sdma.sdma_ring[1].funcs = &sdma_ring_funcs;
}

static int navi32_sdma_ring_init(struct amdgv_adapter *adapt, int ring_id)
{
	struct amdgv_ring *ring = &adapt->sdma.sdma_ring[ring_id];

	ring->me = 0xFFFFFFFF;
	ring->pipe = 0;
	ring->queue = ring_id;

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

static int navi32_sdma_sw_init(struct amdgv_adapter *adapt)
{
	adapt->sdma.num_sdma_rings = 2;
	navi32_sdma_ring_init(adapt, 0);
	navi32_sdma_ring_init(adapt, 1);

	return 0;
}

static int navi32_sdma_sw_fini(struct amdgv_adapter *adapt)
{
	amdgv_ring_fini(&adapt->sdma.sdma_ring[0]);
	amdgv_ring_fini(&adapt->sdma.sdma_ring[1]);

	return 0;
}

static int navi32_enable_sdma(struct amdgv_adapter *adapt)
{
	struct amdgv_ring *ring = &adapt->sdma.sdma_ring[0];
	uint32_t data;
	uint64_t data64;

	/* 1. Disable IB/RB buffer */
	data = RREG32_SOC15(GC, 0, regSDMA0_QUEUE0_IB_CNTL);
	data = REG_SET_FIELD(data, SDMA0_QUEUE0_IB_CNTL, IB_ENABLE, 0);
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_IB_CNTL, data);

	data = RREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_CNTL);
	data = REG_SET_FIELD(data, SDMA0_QUEUE0_RB_CNTL, RB_ENABLE, 0);
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_CNTL, data);

	/* 2. Set ring buffer base */
	data64 = ring->gpu_addr >> 8;
	data = (uint32_t)data64;
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_BASE, data);
	data = (uint32_t)(data64 >> 32);
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_BASE_HI, data);

	/* 3. Set ring buffer size */
	data = RREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_CNTL);
	/* size in log2 of DWORD size = "log2_ring_size" */
	data = REG_SET_FIELD(data, SDMA0_QUEUE0_RB_CNTL, RB_SIZE, ring->log2_ring_size);
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_CNTL, data);

	/* 4. Reset RB registers */
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_RPTR, 0);
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_RPTR_HI, 0);
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_WPTR, 0);
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_WPTR_HI, 0);

	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_RPTR_ADDR_LO, 0);
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_RPTR_ADDR_HI, 0);
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_WPTR_POLL_ADDR_LO, 0);
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_WPTR_POLL_ADDR_HI, 0);

	/* 5. Disable poll */
	data = RREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_CNTL);
	data = REG_SET_FIELD(data, SDMA0_QUEUE0_RB_CNTL, WPTR_POLL_ENABLE, 0);
	data = REG_SET_FIELD(data, SDMA0_QUEUE0_RB_CNTL, F32_WPTR_POLL_ENABLE, 0);
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_CNTL, data);

	/* 6. Enable IB */
	data = RREG32_SOC15(GC, 0, regSDMA0_QUEUE0_IB_CNTL);
	data = REG_SET_FIELD(data, SDMA0_QUEUE0_IB_CNTL, IB_ENABLE, 1);
	data = REG_SET_FIELD(data, SDMA0_QUEUE0_IB_CNTL, IB_SWAP_ENABLE, 0);
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_IB_CNTL, data);

	/* 7. Set up WPTR poll memory */
	data = (uint32_t)(ring->wptr_gpu_addr);
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_WPTR_POLL_ADDR_LO, data);
	data = (uint32_t)(ring->wptr_gpu_addr >> 32);
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_WPTR_POLL_ADDR_HI, data);
	data = RREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_CNTL);
	data = REG_SET_FIELD(data, SDMA0_QUEUE0_RB_CNTL, F32_WPTR_POLL_ENABLE, 1);
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_CNTL, data);

	/* 8. Enable doorbell */
	if (ring->use_doorbell) {
		data = RREG32_SOC15(GC, 0, regSDMA0_QUEUE0_DOORBELL_OFFSET);
		// OFFSET is DWORD aligned, and doorbell is QWORD
		data = REG_SET_FIELD(data, SDMA0_QUEUE0_DOORBELL_OFFSET, OFFSET, ring->doorbell_index);
		WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_DOORBELL_OFFSET, data);

		data = RREG32_SOC15(GC, 0, regSDMA0_QUEUE0_DOORBELL);
		data = REG_SET_FIELD(data, SDMA0_QUEUE0_DOORBELL, ENABLE, 1);
		WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_DOORBELL, data);
	}

	/* 9. Unfreeze engine */
	data = RREG32_SOC15(GC, 0, regSDMA0_FREEZE);
	data = REG_SET_FIELD(data, SDMA0_FREEZE, FREEZE, 0);
	WREG32_SOC15(GC, 0, regSDMA0_FREEZE, data);

	/* 10. Unhalt engine (THREAD0 is controlled by THREAD1 internally) */
	data = RREG32_SOC15(GC, 0, regSDMA0_F32_CNTL);
	data = REG_SET_FIELD(data, SDMA0_F32_CNTL, HALT, 0);
	data = REG_SET_FIELD(data, SDMA0_F32_CNTL, TH1_RESET, 0);
	data = REG_SET_FIELD(data, SDMA0_F32_CNTL, TH0_RESET, 0); /* WA */
	WREG32_SOC15(GC, 0, regSDMA0_F32_CNTL, data);

	/* 11. Enable ring buffer */
	data = RREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_CNTL);
	data = REG_SET_FIELD(data, SDMA0_QUEUE0_RB_CNTL, RB_ENABLE, 1);
	data = REG_SET_FIELD(data, SDMA0_QUEUE0_RB_CNTL, RB_PRIV, 1);
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_CNTL, data);

	return 0;
}

static int navi32_enable_sdma1(struct amdgv_adapter *adapt)
{
	struct amdgv_ring *ring = &adapt->sdma.sdma_ring[1];
	uint32_t data;
	uint64_t data64;

	/* 1. Disable IB/RB buffer */
	data = RREG32_SOC15(GC, 0, regSDMA1_QUEUE0_IB_CNTL);
	data = REG_SET_FIELD(data, SDMA1_QUEUE0_IB_CNTL, IB_ENABLE, 0);
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_IB_CNTL, data);

	data = RREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_CNTL);
	data = REG_SET_FIELD(data, SDMA1_QUEUE0_RB_CNTL, RB_ENABLE, 0);
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_CNTL, data);

	/* 2. Set ring buffer base */
	data64 = ring->gpu_addr >> 8;
	data = (uint32_t)data64;
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_BASE, data);
	data = (uint32_t)(data64 >> 32);
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_BASE_HI, data);

	/* 3. Set ring buffer size */
	data = RREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_CNTL);
	/* size in log2 of DWORD size = "log2_ring_size" */
	data = REG_SET_FIELD(data, SDMA1_QUEUE0_RB_CNTL, RB_SIZE, ring->log2_ring_size);
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_CNTL, data);

	/* 4. Reset RB registers */
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_RPTR, 0);
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_RPTR_HI, 0);
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_WPTR, 0);
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_WPTR_HI, 0);

	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_RPTR_ADDR_LO, 0);
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_RPTR_ADDR_HI, 0);
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_WPTR_POLL_ADDR_LO, 0);
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_WPTR_POLL_ADDR_HI, 0);

	/* 5. Disable poll */
	data = RREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_CNTL);
	data = REG_SET_FIELD(data, SDMA1_QUEUE0_RB_CNTL, WPTR_POLL_ENABLE, 0);
	data = REG_SET_FIELD(data, SDMA1_QUEUE0_RB_CNTL, F32_WPTR_POLL_ENABLE, 0);
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_CNTL, data);

	/* 6. Enable IB */
	data = RREG32_SOC15(GC, 0, regSDMA1_QUEUE0_IB_CNTL);
	data = REG_SET_FIELD(data, SDMA1_QUEUE0_IB_CNTL, IB_ENABLE, 1);
	data = REG_SET_FIELD(data, SDMA1_QUEUE0_IB_CNTL, IB_SWAP_ENABLE, 0);
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_IB_CNTL, data);

	/* 7. Set up WPTR poll memory */
	data = (uint32_t)(ring->wptr_gpu_addr);
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_WPTR_POLL_ADDR_LO, data);
	data = (uint32_t)(ring->wptr_gpu_addr >> 32);
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_WPTR_POLL_ADDR_HI, data);
	data = RREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_CNTL);
	data = REG_SET_FIELD(data, SDMA1_QUEUE0_RB_CNTL, F32_WPTR_POLL_ENABLE, 1);
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_CNTL, data);

	/* 8. Enable doorbell */
	if (ring->use_doorbell) {
		data = RREG32_SOC15(GC, 0, regSDMA1_QUEUE0_DOORBELL_OFFSET);
		// OFFSET is DWORD aligned, and doorbell is QWORD
		data = REG_SET_FIELD(data, SDMA1_QUEUE0_DOORBELL_OFFSET, OFFSET, ring->doorbell_index);
		WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_DOORBELL_OFFSET, data);

		data = RREG32_SOC15(GC, 0, regSDMA1_QUEUE0_DOORBELL);
		data = REG_SET_FIELD(data, SDMA1_QUEUE0_DOORBELL, ENABLE, 1);
		WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_DOORBELL, data);
	}

	/* 9. Unfreeze engine */
	data = RREG32_SOC15(GC, 0, regSDMA1_FREEZE);
	data = REG_SET_FIELD(data, SDMA1_FREEZE, FREEZE, 0);
	WREG32_SOC15(GC, 0, regSDMA1_FREEZE, data);

	/* 10. Unhalt engine (THREAD0 is controlled by THREAD1 internally) */
	data = RREG32_SOC15(GC, 0, regSDMA1_F32_CNTL);
	data = REG_SET_FIELD(data, SDMA1_F32_CNTL, HALT, 0);
	data = REG_SET_FIELD(data, SDMA1_F32_CNTL, TH1_RESET, 0);
	data = REG_SET_FIELD(data, SDMA1_F32_CNTL, TH0_RESET, 0); /* WA */
	WREG32_SOC15(GC, 0, regSDMA1_F32_CNTL, data);

	/* 11. Enable ring buffer */
	data = RREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_CNTL);
	data = REG_SET_FIELD(data, SDMA1_QUEUE0_RB_CNTL, RB_ENABLE, 1);
	data = REG_SET_FIELD(data, SDMA1_QUEUE0_RB_CNTL, RB_PRIV, 1);
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_CNTL, data);

	return 0;
}

static int navi32_disable_sdma(struct amdgv_adapter *adapt)
{
	uint32_t data;
	/* 1. Disable IB */
	data = RREG32_SOC15(GC, 0, regSDMA0_QUEUE0_IB_CNTL);
	data = REG_SET_FIELD(data, SDMA0_QUEUE0_IB_CNTL, IB_ENABLE, 0);
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_IB_CNTL, data);

	/* 2. Disable ring buffer */
	data = RREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_CNTL);
	data = REG_SET_FIELD(data, SDMA0_QUEUE0_RB_CNTL, RB_ENABLE, 0);
	WREG32_SOC15(GC, 0, regSDMA0_QUEUE0_RB_CNTL, data);

	return 0;
}

static int navi32_disable_sdma1(struct amdgv_adapter *adapt)
{
	uint32_t data;
	/* 1. Disable IB */
	data = RREG32_SOC15(GC, 0, regSDMA1_QUEUE0_IB_CNTL);
	data = REG_SET_FIELD(data, SDMA1_QUEUE0_IB_CNTL, IB_ENABLE, 0);
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_IB_CNTL, data);

	/* 2. Disable ring buffer */
	data = RREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_CNTL);
	data = REG_SET_FIELD(data, SDMA1_QUEUE0_RB_CNTL, RB_ENABLE, 0);
	WREG32_SOC15(GC, 0, regSDMA1_QUEUE0_RB_CNTL, data);

	return 0;
}

static int navi32_sdma_hw_init_internal_set(struct amdgv_adapter *adapt)
{
	struct amdgv_ring *ring = NULL;
	int ret = 0;

	ring = &adapt->sdma.sdma_ring[0];
	ret = amdgv_ring_init_set(adapt, ring);

	if (ret)
		return ret;

	ring = &adapt->sdma.sdma_ring[1];
	ret = amdgv_ring_init_set(adapt, ring);

	return ret;
}

static int navi32_sdma_hw_init(struct amdgv_adapter *adapt)
{
	struct amdgv_ring *ring;
	int ret = 0;

	ret = navi32_sdma_hw_init_internal_set(adapt);

	if (in_whole_gpu_reset()) {
		// Clear the ring
		ring = &adapt->sdma.sdma_ring[0];
		amdgv_ring_clear_ring(ring);
		ring->wptr = 0;

		ring = &adapt->sdma.sdma_ring[1];
		amdgv_ring_clear_ring(ring);
		ring->wptr = 0;
	}

	if (ret)
		return ret;

	navi32_enable_sdma(adapt);
	navi32_enable_sdma1(adapt);

	return 0;
}

static int navi32_sdma_hw_fini(struct amdgv_adapter *adapt)
{
	navi32_disable_sdma(adapt);
	navi32_disable_sdma1(adapt);

	return 0;
}

struct amdgv_init_func navi32_sdma_func = {
	.name = "navi32_sdma_func",
	.is_engine = true,
	.sw_init = navi32_sdma_sw_init,
	.sw_fini = navi32_sdma_sw_fini,
	.hw_init = navi32_sdma_hw_init,
	.hw_fini = navi32_sdma_hw_fini,
	.hw_live_init = navi32_sdma_hw_init_internal_set,
};
